#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include "storage/shard.hpp"
#include "persistence/aof_writer.hpp"

namespace mini_redis {

class StorageEngine {
public:
    explicit StorageEngine(size_t shard_count = 64);

    bool get(const std::string& key, std::string& value);
    void set(const std::string& key, std::string value);
    void set_with_ttl(const std::string& key, std::string value, uint64_t ttl_seconds);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    bool expire(const std::string& key, uint64_t ttl_seconds);
    int64_t ttl(const std::string& key);
    void enable_aof(const std::string& filename);

private:
    std::vector<Shard> shards_;

    Shard& shard_for(const std::string& key);
    uint64_t now_seconds() const;

    std::unique_ptr<AOFWriter> aof_writer_;
};

} // namespace mini_redis
