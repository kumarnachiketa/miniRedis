#include "protocol/response.hpp"

namespace protocol {

std::string RespResponse::ok() {
    return "+OK\r\n";
}

std::string RespResponse::error(
        const std::string& msg) {
    return "-" + msg + "\r\n";
}

std::string RespResponse::bulk(
        const std::string& val) {

    return "$" +
        std::to_string(val.size()) +
        "\r\n" +
        val +
        "\r\n";
}

std::string RespResponse::null() {
    return "$-1\r\n";
}

std::string RespResponse::integer(int64_t value) {
    return ":" + std::to_string(value) + "\r\n";
}

}
