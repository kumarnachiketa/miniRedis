#include "net/server.hpp"
#include "net/connection.hpp"
#include "protocol/executor.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <vector>
#include <cerrno>

namespace net {

namespace {

constexpr int MAX_EVENTS = 256;

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0)
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

} // namespace

Server::Server(int port, mini_redis::StorageEngine& storage, size_t worker_threads)
    : port_(port),
      storage_(storage),
      pool_(worker_threads),
      listen_fd_(-1),
      kq_(-1) {}

Server::~Server() {
    if (kq_ >= 0) close(kq_);
    if (listen_fd_ >= 0) close(listen_fd_);
}

bool Server::start() {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        perror("socket");
        return false;
    }

    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return false;
    }

    if (listen(listen_fd_, 1024) < 0) {
        perror("listen");
        return false;
    }

    kq_ = kqueue();
    if (kq_ < 0) {
        perror("kqueue");
        return false;
    }

    struct kevent ev;
    EV_SET(&ev, listen_fd_, EVFILT_READ, EV_ADD, 0, 0, nullptr);
    if (kevent(kq_, &ev, 1, nullptr, 0, nullptr) < 0) {
        perror("kevent listen");
        return false;
    }

    std::cout << "miniRedis listening on port " << port_ << " (kqueue + thread pool)\n";
    return true;
}

void Server::submit_command(int fd, std::vector<std::string> cmd) {
    mini_redis::StorageEngine* storage = &storage_;
    pool_.enqueue([this, fd, cmd = std::move(cmd), storage]() {
        std::string response = mini_redis::protocol::execute_command(*storage, cmd);
        push_pending_response(fd, std::move(response));
    });
}

void Server::push_pending_response(int fd, std::string response) {
    std::lock_guard lock(response_mutex_);
    response_queue_.emplace(fd, std::move(response));
}

void Server::drain_response_queue(std::vector<struct kevent>& changelist) {
    std::queue<std::pair<int, std::string>> batch;
    {
        std::lock_guard lock(response_mutex_);
        batch.swap(response_queue_);
    }
    while (!batch.empty()) {
        int fd = batch.front().first;
        std::string response = std::move(batch.front().second);
        batch.pop();
        auto it = connections_.find(fd);
        if (it != connections_.end()) {
            it->second->add_pending_response(std::move(response));
            if (write_interest_.count(fd) == 0) {
                write_interest_.insert(fd);
                struct kevent ev;
                EV_SET(&ev, fd, EVFILT_WRITE, EV_ADD, 0, 0, nullptr);
                changelist.push_back(ev);
            }
        }
    }
}

void Server::accept_client() {
    sockaddr_in client{};
    socklen_t len = sizeof(client);
    int client_fd = accept(listen_fd_, (sockaddr*)&client, &len);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            perror("accept");
        return;
    }
    set_nonblocking(client_fd);

    auto submit_fn = [this](int fd, std::vector<std::string> cmd) {
        submit_command(fd, std::move(cmd));
    };
    connections_.emplace(client_fd, std::make_unique<Connection>(client_fd, submit_fn));

    struct kevent ev;
    EV_SET(&ev, client_fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
    kevent(kq_, &ev, 1, nullptr, 0, nullptr);
}

void Server::run() {
    std::vector<struct kevent> events(MAX_EVENTS);
    std::vector<int> dead;
    std::vector<struct kevent> changelist;
    changelist.reserve(MAX_EVENTS);

    while (true) {
        changelist.clear();
        drain_response_queue(changelist);

        int n = kevent(kq_,
                      changelist.empty() ? nullptr : changelist.data(),
                      static_cast<int>(changelist.size()),
                      events.data(),
                      static_cast<int>(events.size()),
                      nullptr);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("kevent");
            continue;
        }

        dead.clear();
        changelist.clear();

        for (int i = 0; i < n; ++i) {
            int fd = static_cast<int>(events[i].ident);
            if (fd == listen_fd_) {
                accept_client();
                continue;
            }

            auto it = connections_.find(fd);
            if (it == connections_.end()) continue;

            Connection* conn = it->second.get();
            bool alive = true;

            if (events[i].filter == EVFILT_READ) {
                alive = conn->handle_read();
                if (alive && conn->wants_write() && write_interest_.count(fd) == 0) {
                    write_interest_.insert(fd);
                    struct kevent ev;
                    EV_SET(&ev, fd, EVFILT_WRITE, EV_ADD, 0, 0, nullptr);
                    changelist.push_back(ev);
                }
            } else if (events[i].filter == EVFILT_WRITE) {
                alive = conn->handle_write();
                if (alive && !conn->wants_write() && write_interest_.count(fd) != 0) {
                    write_interest_.erase(fd);
                    struct kevent ev;
                    EV_SET(&ev, fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
                    changelist.push_back(ev);
                }
            }

            if (!alive)
                dead.push_back(fd);
        }

        if (!changelist.empty()) {
            kevent(kq_, changelist.data(), static_cast<int>(changelist.size()), nullptr, 0, nullptr);
            changelist.clear();
        }

        for (int fd : dead) {
            write_interest_.erase(fd);
            struct kevent ev;
            EV_SET(&ev, fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
            kevent(kq_, &ev, 1, nullptr, 0, nullptr);
            connections_.erase(fd);
        }
    }
}

} // namespace net
