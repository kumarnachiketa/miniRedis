#include "persistence/aof_writer.hpp"

namespace mini_redis {

AOFWriter::AOFWriter(const std::string& filename)
    : file_(filename, std::ios::app) {}

void AOFWriter::append_set(
    const std::string& key,
    const std::string& value
) {
    std::lock_guard lock(mutex_);
    file_ << "SET " << key << " " << value << "\n";
    file_.flush();
}

void AOFWriter::append_setex(
    const std::string& key,
    uint64_t ttl,
    const std::string& value
) {
    std::lock_guard lock(mutex_);
    file_ << "SETEX " << key << " " << ttl << " " << value << "\n";
    file_.flush();
}

void AOFWriter::append_del(const std::string& key) {
    std::lock_guard lock(mutex_);
    file_ << "DEL " << key << "\n";
    file_.flush();
}

} // namespace mini_redis
