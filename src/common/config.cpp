#include "common/config.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace mini_redis {

namespace {

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end == std::string::npos ? std::string::npos : end - start + 1);
}

void set_value(ServerConfig& c, const std::string& key, const std::string& value) {
    if (key == "port") {
        try {
            c.port = std::stoi(value);
            if (c.port <= 0 || c.port > 65535) c.port = 6379;
        } catch (...) {}
    } else if (key == "aof_file") {
        c.aof_file = trim(value);
        if (c.aof_file.empty()) c.aof_file = "aof.log";
    } else if (key == "shards") {
        try {
            size_t n = static_cast<size_t>(std::stoull(value));
            if (n > 0 && n <= 1024) c.shard_count = n;
        } catch (...) {}
    } else if (key == "aof_fsync") {
        std::string v = value;
        if (v == "no" || v == "0" || v == "false") c.aof_fsync_every_write = false;
    }
}

} // namespace

ServerConfig load_config(const std::string& path) {
    ServerConfig c;
    std::ifstream f(path);
    if (!f) return c;

    std::string line;
    while (std::getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        size_t sep = line.find('=');
        if (sep == std::string::npos) sep = line.find(' ');
        if (sep == std::string::npos) continue;

        std::string key = trim(line.substr(0, sep));
        std::string value = trim(line.substr(sep + 1));
        if (!key.empty())
            set_value(c, key, value);
    }
    return c;
}

} // namespace mini_redis
