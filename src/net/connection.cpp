#include "net/connection.hpp"

#include <unistd.h>
#include <errno.h>
#include <cstdint>

namespace net {

Connection::Connection(int fd, SubmitFn submit_fn)
    : fd_(fd),
      submit_fn_(std::move(submit_fn)) {}

Connection::~Connection() {
    if (fd_ >= 0)
        close(fd_);
}

int Connection::fd() const {
    return fd_;
}

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

bool Connection::process_buffer() {
    size_t pos = 0;

    while (true) {
        std::vector<std::string> cmd;
        if (!parser_.parse_array(read_buffer_, pos, cmd))
            break;
        if (cmd.empty())
            continue;
        submit_fn_(fd_, std::move(cmd));
    }

    read_buffer_.erase(0, pos);
    return true;
}

void Connection::add_pending_response(std::string response) {
    std::lock_guard lock(write_mutex_);
    write_buffer_ += std::move(response);
}

bool Connection::handle_write() {
    std::lock_guard lock(write_mutex_);
    if (write_buffer_.empty())
        return true;

    ssize_t n = write(fd_, write_buffer_.data(), write_buffer_.size());
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return true;
        return false;
    }
    write_buffer_.erase(0, static_cast<size_t>(n));
    return true;
}

bool Connection::wants_write() const {
    std::lock_guard lock(write_mutex_);
    return !write_buffer_.empty();
}

} // namespace net
