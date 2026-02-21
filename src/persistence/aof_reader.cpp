#include "persistence/aof_reader.hpp"
#include "storage/storage_engine.hpp"

#include <fstream>
#include <sstream>

namespace mini_redis {

AOFReader::AOFReader(const std::string& filename)
    : filename_(filename) {}

void AOFReader::replay(StorageEngine& engine) {
    std::ifstream in(filename_);
    if (!in.is_open()) {
        return; // No AOF yet
    }

    std::string line;
    while (std::getline(in, line)) {
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "SET") {
            std::string key, value;
            iss >> key >> value;
            engine.set(key, value);
        }
        else if (cmd == "SETEX") {
            std::string key, value;
            uint64_t ttl;
            iss >> key >> ttl >> value;
            engine.set_with_ttl(key, value, ttl);
        }
        else if (cmd == "DEL") {
            std::string key;
            iss >> key;
            engine.del(key);
        }
        else if (cmd == "EXPIRE") {
            std::string key;
            uint64_t ttl;
            iss >> key >> ttl;
            engine.expire(key, ttl);
        }
    }
}

} // namespace mini_redis
