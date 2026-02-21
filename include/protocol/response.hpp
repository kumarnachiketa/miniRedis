#pragma once
#include <string>
#include <vector>

namespace protocol {

class RespResponse {
public:
    static std::string ok();
    static std::string error(const std::string&);
    static std::string bulk(const std::string&);
    static std::string null();
    static std::string integer(int64_t value);
    static std::string array(const std::vector<std::string>& elements);
};

}
