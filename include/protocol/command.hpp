#pragma once

#include <string>
#include <vector>

namespace mini_redis::protocol {

enum class CommandType {
    GET,
    SET,
    SETEX,
    DEL,
    UNKNOWN
};

struct Command {
    CommandType type{CommandType::UNKNOWN};
    std::vector<std::string> args;
};

} // namespace mini_redis::protocol
