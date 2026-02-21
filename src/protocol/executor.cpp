#include "protocol/executor.hpp"
#include "protocol/response.hpp"
#include "storage/storage_engine.hpp"

#include <cstdlib>
#include <cstdint>

namespace mini_redis::protocol {

namespace {

std::string execute_impl(mini_redis::StorageEngine& storage,
                         const std::vector<std::string>& cmd) {
    if (cmd.empty()) return ::protocol::RespResponse::error("empty command");
    const std::string& op = cmd[0];

    if (op == "PING")
        return ::protocol::RespResponse::bulk("PONG");

    if (op == "SET" && cmd.size() >= 3) {
        storage.set(cmd[1], cmd[2]);
        return ::protocol::RespResponse::ok();
    }

    if (op == "GET" && cmd.size() >= 2) {
        std::string value;
        if (!storage.get(cmd[1], value))
            return ::protocol::RespResponse::null();
        return ::protocol::RespResponse::bulk(value);
    }

    if (op == "SETEX" && cmd.size() >= 4) {
        uint64_t ttl = 0;
        try { ttl = static_cast<uint64_t>(std::stoull(cmd[2])); }
        catch (...) { return ::protocol::RespResponse::error("invalid expire time"); }
        storage.set_with_ttl(cmd[1], cmd[3], ttl);
        return ::protocol::RespResponse::ok();
    }

    if (op == "DEL" && cmd.size() >= 2) {
        int64_t removed = 0;
        for (size_t i = 1; i < cmd.size(); ++i)
            if (storage.del(cmd[i])) ++removed;
        return ::protocol::RespResponse::integer(removed);
    }

    if (op == "EXISTS" && cmd.size() >= 2) {
        int64_t count = 0;
        for (size_t i = 1; i < cmd.size(); ++i)
            if (storage.exists(cmd[i])) ++count;
        return ::protocol::RespResponse::integer(count);
    }

    if (op == "EXPIRE" && cmd.size() >= 3) {
        uint64_t ttl = 0;
        try { ttl = static_cast<uint64_t>(std::stoull(cmd[2])); }
        catch (...) { return ::protocol::RespResponse::error("invalid expire time"); }
        return ::protocol::RespResponse::integer(storage.expire(cmd[1], ttl) ? 1 : 0);
    }

    if (op == "TTL" && cmd.size() >= 2)
        return ::protocol::RespResponse::integer(storage.ttl(cmd[1]));

    if (op == "KEYS" && cmd.size() >= 2) {
        auto key_list = storage.keys(cmd[1]);
        return ::protocol::RespResponse::array(key_list);
    }

    return ::protocol::RespResponse::error("unknown command");
}

} // namespace

std::string execute_command(mini_redis::StorageEngine& storage,
                            const std::vector<std::string>& cmd) {
    return execute_impl(storage, cmd);
}

} // namespace mini_redis::protocol
