#pragma once

#include <cstddef>
#include <string>

namespace mini_redis {

struct ServerConfig {
    int port = 6379;
    std::string aof_file = "aof.log";
    size_t shard_count = 64;
};

// Load from file (key=value or key value per line). Missing keys keep defaults.
// Returns config with defaults if file is missing or empty.
ServerConfig load_config(const std::string& path);

} // namespace mini_redis
