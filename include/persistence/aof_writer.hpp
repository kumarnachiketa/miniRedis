#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <cstdint>

namespace mini_redis {

class AOFWriter {
public:
    AOFWriter(const std::string& filename, bool flush_on_each = true);

    void append_set(const std::string& key, const std::string& value);
    void append_setex(const std::string& key, uint64_t ttl, const std::string& value);
    void append_del(const std::string& key);
    void append_expire(const std::string& key, uint64_t ttl_seconds);

private:
    void maybe_flush();

    std::ofstream file_;
    std::mutex mutex_;
    bool flush_on_each_;
};

} // namespace mini_redis
