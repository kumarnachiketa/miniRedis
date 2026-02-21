#pragma once

#include <unordered_map>
#include <shared_mutex>
#include <string>
#include <cstdint>
#include "storage/value.hpp"

namespace mini_redis {

class Shard {
public:
    using Map = std::unordered_map<std::string, Value>;

    bool get(const std::string& key, Value& out, uint64_t now);
    void set(const std::string& key, Value value);
    bool del(const std::string& key);
    bool exists(const std::string& key, uint64_t now);
    bool set_expire(const std::string& key, uint64_t expire_at, uint64_t now);
    int64_t ttl(const std::string& key, uint64_t now);

private:
    Map map_;
    mutable std::shared_mutex mutex_;
};

} // namespace mini_redis
