#pragma once

#include <unordered_map>
#include <memory>
#include <set>
#include <queue>
#include <mutex>
#include <sys/event.h>

#include "storage/storage_engine.hpp"
#include "net/connection.hpp"
#include "concurrency/thread_pool.hpp"

namespace net {

class Server {
public:
    Server(int port,
          mini_redis::StorageEngine& storage,
          size_t worker_threads = 4);

    ~Server();

    bool start();
    void run();

private:
    void accept_client();
    void submit_command(int fd, std::vector<std::string> cmd);
    void push_pending_response(int fd, std::string response);
    void drain_response_queue(std::vector<struct kevent>& changelist);

    int port_;
    int listen_fd_;
    int kq_;

    mini_redis::StorageEngine& storage_;
    mini_redis::concurrency::ThreadPool pool_;

    std::mutex response_mutex_;
    std::queue<std::pair<int, std::string>> response_queue_;

    std::unordered_map<int, std::unique_ptr<Connection>> connections_;
    std::set<int> write_interest_;
};

}
