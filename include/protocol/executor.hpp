#pragma once

#include <string>
#include <vector>

namespace mini_redis {
class StorageEngine;
}

namespace mini_redis::protocol {

// Execute a single command and return RESP response string. Thread-safe if storage is.
std::string execute_command(mini_redis::StorageEngine& storage,
                            const std::vector<std::string>& cmd);

} // namespace mini_redis::protocol
