#include "net/connection.hpp"

#include <unistd.h>
#include <errno.h>
#include <cstdlib>
#include <cstdint>

namespace net {

/* ===============================
   Constructor / Destructor
=============================== */

Connection::Connection(
        int fd,
        mini_redis::StorageEngine& storage)
    : fd_(fd),
      storage_(storage) {}

Connection::~Connection() {
    if (fd_ >= 0)
        close(fd_);
}

int Connection::fd() const {
    return fd_;
}


/* ===============================
   READ HANDLER
=============================== */

bool Connection::handle_read() {

    char buffer[4096];

    ssize_t n = read(fd_, buffer, sizeof(buffer));

    if (n > 0) {
        read_buffer_.append(buffer, n);

            process_buffer();

        return true;
    }

    if (n == 0)
        return false;

    if (errno == EAGAIN || errno == EWOULDBLOCK)
        return true;
    return false;
}


/* ===============================
   COMMAND PROCESSING
=============================== */

bool Connection::process_buffer() {

    size_t pos = 0;

    while (true) {

        std::vector<std::string> cmd;

        if (!parser_.parse_array(read_buffer_, pos, cmd))
            break;   // need more data

        if (cmd.empty())
            continue;

        std::string response;
        const std::string& op = cmd[0];

        /* ---------- PING ---------- */
        if (op == "PING") {
            response =
                protocol::RespResponse::bulk("PONG");
        }

        /* ---------- SET ---------- */
        else if (op == "SET" && cmd.size() >= 3) {
            storage_.set(cmd[1], cmd[2]);
            response =
                protocol::RespResponse::ok();
        }

        /* ---------- GET ---------- */
        else if (op == "GET" && cmd.size() >= 2) {
            std::string value;
            if (!storage_.get(cmd[1], value))
                response = protocol::RespResponse::null();
            else
                response = protocol::RespResponse::bulk(value);
        }

        /* ---------- SETEX ---------- */
        else if (op == "SETEX" && cmd.size() >= 4) {
            uint64_t ttl = 0;
            bool valid = true;
            try {
                ttl = static_cast<uint64_t>(std::stoull(cmd[2]));
            } catch (...) {
                valid = false;
            }
            if (!valid)
                response = protocol::RespResponse::error("invalid expire time");
            else {
                storage_.set_with_ttl(cmd[1], cmd[3], ttl);
                response = protocol::RespResponse::ok();
            }
        }

        /* ---------- DEL ---------- */
        else if (op == "DEL" && cmd.size() >= 2) {
            int64_t removed = 0;
            for (size_t i = 1; i < cmd.size(); ++i) {
                if (storage_.del(cmd[i]))
                    ++removed;
            }
            response = protocol::RespResponse::integer(removed);
        }

        /* ---------- EXISTS ---------- */
        else if (op == "EXISTS" && cmd.size() >= 2) {
            int64_t count = 0;
            for (size_t i = 1; i < cmd.size(); ++i) {
                if (storage_.exists(cmd[i])) ++count;
            }
            response = protocol::RespResponse::integer(count);
        }

        /* ---------- EXPIRE ---------- */
        else if (op == "EXPIRE" && cmd.size() >= 3) {
            uint64_t ttl = 0;
            bool valid = true;
            try {
                ttl = static_cast<uint64_t>(std::stoull(cmd[2]));
            } catch (...) {
                valid = false;
            }
            if (!valid)
                response = protocol::RespResponse::error("invalid expire time");
            else
                response = protocol::RespResponse::integer(storage_.expire(cmd[1], ttl) ? 1 : 0);
        }

        /* ---------- TTL ---------- */
        else if (op == "TTL" && cmd.size() >= 2) {
            int64_t t = storage_.ttl(cmd[1]);
            response = protocol::RespResponse::integer(t);
        }

        /* ---------- KEYS ---------- */
        else if (op == "KEYS" && cmd.size() >= 2) {
            std::vector<std::string> key_list = storage_.keys(cmd[1]);
            response = protocol::RespResponse::array(key_list);
        }

        else {
            response =
                protocol::RespResponse::error(
                    "unknown command");
        }

        write_buffer_ += response;
    }

    // remove processed bytes
    read_buffer_.erase(0, pos);

    if (!write_buffer_.empty())
        handle_write();

    return true;
}



/* ===============================
   WRITE HANDLER
=============================== */

bool Connection::handle_write() {

    if (write_buffer_.empty())
        return true;

    ssize_t n =
        write(fd_,
              write_buffer_.data(),
              write_buffer_.size());

    if (n < 0) {

        if (errno == EWOULDBLOCK ||
            errno == EAGAIN)
            return true;

        perror("write");
        return false;
    }

    write_buffer_.erase(0, n);

    return true;
}


/* ===============================
   WRITE INTEREST
=============================== */

bool Connection::wants_write() const {
    return !write_buffer_.empty();
}

} // namespace net
