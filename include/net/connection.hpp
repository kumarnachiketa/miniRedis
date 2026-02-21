#pragma once

#include <string>
#include <vector>
#include <functional>
#include <mutex>

#include "protocol/parser.hpp"
#include "protocol/response.hpp"
#include "storage/storage_engine.hpp"

namespace net {

class Connection {
public:
    using SubmitFn = std::function<void(int fd, std::vector<std::string> cmd)>;

    Connection(int fd, SubmitFn submit_fn);

    ~Connection();

    int fd() const;

    bool handle_read();
    bool handle_write();
    bool wants_write() const;

    // Called from main thread when worker pushes a response
    void add_pending_response(std::string response);

private:
    bool process_buffer();

    int fd_;
    SubmitFn submit_fn_;

    protocol::RespParser parser_;
    std::string read_buffer_;

    mutable std::mutex write_mutex_;
    std::string write_buffer_;
};

}
