#include "persistence/aof_writer.hpp"

namespace mini_redis {

AOFWriter::AOFWriter(const std::string& filename, bool flush_on_each)
    : file_(filename, std::ios::app), flush_on_each_(flush_on_each) {}

void AOFWriter::maybe_flush() {
    if (flush_on_each_)
        file_.flush();
}

void AOFWriter::append_set(
    const std::string& key,
    const std::string& value
) {
    std::lock_guard lock(mutex_);
    file_ << "SET " << key << " " << value << "\n";
    maybe_flush();
}

void AOFWriter::append_setex(
    const std::string& key,
    uint64_t ttl,
    const std::string& value
) {
    std::lock_guard lock(mutex_);
    file_ << "SETEX " << key << " " << ttl << " " << value << "\n";
    maybe_flush();
}

void AOFWriter::append_del(const std::string& key) {
    std::lock_guard lock(mutex_);
    file_ << "DEL " << key << "\n";
    maybe_flush();
}

void AOFWriter::append_expire(const std::string& key, uint64_t ttl_seconds) {
    std::lock_guard lock(mutex_);
    file_ << "EXPIRE " << key << " " << ttl_seconds << "\n";
    maybe_flush();
}

} // namespace mini_redis
