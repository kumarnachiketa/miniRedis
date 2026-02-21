#pragma once

#include <string>
#include <vector>

#include "protocol/parser.hpp"
#include "protocol/response.hpp"
#include "storage/storage_engine.hpp"

namespace net {

class Connection {
public:
    Connection(
        int fd,
        mini_redis::StorageEngine& storage);

    ~Connection();

    int fd() const;

    // Event loop calls these
    bool handle_read();
    bool handle_write();
    bool wants_write() const;

private:
    bool process_buffer();

private:
    int fd_;
    mini_redis::StorageEngine& storage_;

    protocol::RespParser parser_;

    // incoming data
    std::string read_buffer_;

    // outgoing data
    std::string write_buffer_;
};

}
