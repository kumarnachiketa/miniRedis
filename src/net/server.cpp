#include "net/server.hpp"
#include "net/connection.hpp"

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <vector>

namespace net {

/* ===============================
   Constructor / Destructor
=============================== */

Server::Server(
        int port,
        mini_redis::StorageEngine& storage)
    : port_(port),
      storage_(storage),
      listen_fd_(-1) {}

Server::~Server() {
    if (listen_fd_ >= 0)
        close(listen_fd_);
}


/* ===============================
   START SERVER
=============================== */

bool Server::start() {

    listen_fd_ =
        socket(AF_INET, SOCK_STREAM, 0);

    if (listen_fd_ < 0) {
        perror("socket");
        return false;
    }

    int opt = 1;
    setsockopt(
        listen_fd_,
        SOL_SOCKET,
        SO_REUSEADDR,
        &opt,
        sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(
            listen_fd_,
            (sockaddr*)&addr,
            sizeof(addr)) < 0) {

        perror("bind");
        return false;
    }

    if (listen(listen_fd_, 128) < 0) {
        perror("listen");
        return false;
    }

    std::cout
        << "miniRedis listening on port "
        << port_
        << std::endl;

    return true;
}


/* ===============================
   ACCEPT CLIENT
=============================== */

void Server::accept_client() {

    sockaddr_in client{};
    socklen_t len = sizeof(client);

    int client_fd =
        accept(listen_fd_,
               (sockaddr*)&client,
               &len);

    if (client_fd < 0) {
        perror("accept");
        return;
    }

    std::cout
        << "Client connected fd="
        << client_fd << std::endl;

    connections_.emplace(
        client_fd,
        std::make_unique<Connection>(
            client_fd,
            storage_));
}


/* ===============================
   EVENT LOOP (SELECT)
=============================== */

void Server::run() {

    fd_set readfds;
    fd_set writefds;

    while (true) {

        FD_ZERO(&readfds);
        FD_ZERO(&writefds);

        FD_SET(listen_fd_, &readfds);

        int max_fd = listen_fd_;

        /* register connections */
        for (auto& [fd, conn] : connections_) {

            FD_SET(fd, &readfds);

            // ⭐ ONLY when needed
            if (conn->wants_write())
                FD_SET(fd, &writefds);

            if (fd > max_fd)
                max_fd = fd;
        }

        int activity =
            select(max_fd + 1,
                   &readfds,
                   &writefds,
                   nullptr,
                   nullptr);

        if (activity < 0) {
            perror("select");
            continue;
        }

        /* new client */
        if (FD_ISSET(listen_fd_, &readfds))
            accept_client();

        std::vector<int> dead;

        for (auto& [fd, conn] : connections_) {

            bool alive = true;

            if (FD_ISSET(fd, &readfds))
                alive &= conn->handle_read();

            if (alive &&
                FD_ISSET(fd, &writefds))
                alive &= conn->handle_write();

            if (!alive)
                dead.push_back(fd);
        }

        for (int fd : dead) {
            connections_.erase(fd);
            std::cout
                << "Client removed fd="
                << fd << std::endl;
        }
    }
}

} // namespace net
