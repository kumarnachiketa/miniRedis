#pragma once

#include <unordered_map>
#include <memory>
#include <set>

#include "storage/storage_engine.hpp"
#include "net/connection.hpp"

namespace net {

class Server {
public:
    Server(int port,
           mini_redis::StorageEngine& storage);

    ~Server();

    bool start();
    void run();

private:
    void accept_client();

    int port_;
    int listen_fd_;
    int kq_;  // kqueue fd (macOS/BSD)

    mini_redis::StorageEngine& storage_;

    std::unordered_map<
        int,
        std::unique_ptr<Connection>
    > connections_;
    std::set<int> write_interest_;  // fds currently in kqueue for EVFILT_WRITE
};

}
