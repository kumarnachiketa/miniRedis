#pragma once
#include <string>

namespace protocol {

class RespResponse {
public:
    static std::string ok();
    static std::string error(const std::string&);
    static std::string bulk(const std::string&);
    static std::string null();
};

}
