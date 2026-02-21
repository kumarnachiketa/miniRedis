#include "storage/storage_engine.hpp"
#include "persistence/aof_reader.hpp"
#include <functional>
#include <chrono>

namespace mini_redis {

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


void StorageEngine::enable_aof(const std::string& filename) {
    AOFReader reader(filename);
    reader.replay(*this);

    aof_writer_ = std::make_unique<AOFWriter>(filename);
}

} // namespace mini_redis
