#include "storage/storage_engine.hpp"
#include "persistence/aof_reader.hpp"
#include <functional>
#include <chrono>
#include <algorithm>

namespace mini_redis {

namespace {

// Simple glob: * = any chars, ? = one char (Redis-style)
bool match_pattern(const std::string& pattern, const std::string& key) {
    size_t pi = 0, ki = 0;
    size_t star_pi = std::string::npos, star_ki = std::string::npos;
    while (ki < key.size()) {
        if (pi < pattern.size() && (pattern[pi] == '*' || pattern[pi] == '?')) {
            if (pattern[pi] == '*') {
                star_pi = pi;
                star_ki = ki;
                ++pi;
                continue;
            }
            ++pi;
            ++ki;
            continue;
        }
        if (pi < pattern.size() && pattern[pi] == key[ki]) {
            ++pi;
            ++ki;
            continue;
        }
        if (star_pi != std::string::npos) {
            pi = star_pi + 1;
            ki = ++star_ki;
            continue;
        }
        return false;
    }
    while (pi < pattern.size() && pattern[pi] == '*') ++pi;
    return pi == pattern.size();
}

} // namespace

StorageEngine::StorageEngine(size_t shard_count)
    : shards_(shard_count) {}

Shard& StorageEngine::shard_for(const std::string& key) {
    size_t idx = std::hash<std::string>{}(key) % shards_.size();
    return shards_[idx];
}

uint64_t StorageEngine::now_seconds() const {
    using namespace std::chrono;
    return duration_cast<seconds>(
        steady_clock::now().time_since_epoch()
    ).count();
}

bool StorageEngine::get(const std::string& key, std::string& value) {
    Value v;
    if (!shard_for(key).get(key, v, now_seconds())) {
        return false;
    }
    value = v.data;
    return true;
}

void StorageEngine::set(const std::string& key, std::string value) {
    shard_for(key).set(key, Value{value});
    if (aof_writer_) {
        aof_writer_->append_set(key, value);
    }
}


void StorageEngine::set_with_ttl(
    const std::string& key,
    std::string value,
    uint64_t ttl_seconds
) {
    uint64_t expire_at = now_seconds() + ttl_seconds;
    shard_for(key).set(key, Value{value, expire_at});

    if (aof_writer_) {
        aof_writer_->append_setex(key, ttl_seconds, value);
    }
}


bool StorageEngine::del(const std::string& key) {
    bool res = shard_for(key).del(key);
    if (res && aof_writer_) {
        aof_writer_->append_del(key);
    }
    return res;
}

bool StorageEngine::exists(const std::string& key) {
    return shard_for(key).exists(key, now_seconds());
}

bool StorageEngine::expire(const std::string& key, uint64_t ttl_seconds) {
    uint64_t expire_at = now_seconds() + ttl_seconds;
    bool res = shard_for(key).set_expire(key, expire_at, now_seconds());
    if (res && aof_writer_) {
        aof_writer_->append_expire(key, ttl_seconds);
    }
    return res;
}

int64_t StorageEngine::ttl(const std::string& key) {
    return shard_for(key).ttl(key, now_seconds());
}

std::vector<std::string> StorageEngine::keys(const std::string& pattern) {
    std::vector<std::string> out;
    uint64_t now = now_seconds();
    for (auto& shard : shards_)
        shard.keys(now, out);
    if (pattern != "*") {
        std::vector<std::string> filtered;
        for (const auto& k : out) {
            if (match_pattern(pattern, k))
                filtered.push_back(k);
        }
        return filtered;
    }
    return out;
}

void StorageEngine::enable_aof(const std::string& filename, bool flush_each_write) {
    AOFReader reader(filename);
    reader.replay(*this);
    aof_writer_ = std::make_unique<AOFWriter>(filename, flush_each_write);
}

} // namespace mini_redis
