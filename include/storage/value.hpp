#pragma once

#include <string>
#include <cstdint>

namespace mini_redis {

struct Value {
    std::string data;
    uint64_t expire_at; // 0 = never expires

    Value() : expire_at(0) {}

    Value(std::string d, uint64_t exp = 0)
        : data(std::move(d)), expire_at(exp) {}

    bool is_expired(uint64_t now) const {
        return expire_at != 0 && expire_at <= now;
    }
};

} // namespace mini_redis
