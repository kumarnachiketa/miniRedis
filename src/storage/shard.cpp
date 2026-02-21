#include "storage/shard.hpp"

namespace mini_redis {

bool Shard::get(const std::string& key, Value& out, uint64_t now) {
    {
        std::shared_lock lock(mutex_);
        auto it = map_.find(key);
        if (it == map_.end()) {
            return false;
        }

        if (!it->second.is_expired(now)) {
            out = it->second;
            return true;
        }
    }

    // If expired, delete lazily
    {
        std::unique_lock lock(mutex_);
        auto it = map_.find(key);
        if (it != map_.end() && it->second.is_expired(now)) {
            map_.erase(it);
        }
    }

    return false;
}

void Shard::set(const std::string& key, Value value) {
    std::unique_lock lock(mutex_);
    map_[key] = std::move(value);
}

bool Shard::del(const std::string& key) {
    std::unique_lock lock(mutex_);
    return map_.erase(key) > 0;
}

} // namespace mini_redis
