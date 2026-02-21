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

bool Shard::exists(const std::string& key, uint64_t now) {
    std::shared_lock lock(mutex_);
    auto it = map_.find(key);
    if (it == map_.end()) return false;
    if (it->second.is_expired(now)) return false;
    return true;
}

bool Shard::set_expire(const std::string& key, uint64_t expire_at, uint64_t now) {
    std::unique_lock lock(mutex_);
    auto it = map_.find(key);
    if (it == map_.end() || it->second.is_expired(now)) return false;
    it->second.expire_at = expire_at;
    return true;
}

int64_t Shard::ttl(const std::string& key, uint64_t now) {
    std::shared_lock lock(mutex_);
    auto it = map_.find(key);
    if (it == map_.end()) return -2;
    if (it->second.is_expired(now)) return -2;
    if (it->second.expire_at == 0) return -1;
    auto rem = static_cast<int64_t>(it->second.expire_at - now);
    return rem > 0 ? rem : 0;
}

} // namespace mini_redis
