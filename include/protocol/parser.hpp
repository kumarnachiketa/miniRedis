#pragma once
#include <string>
#include <vector>

namespace protocol {

class RespParser {
public:
    bool parse_array(const std::string& in,
                     size_t& pos,
                     std::vector<std::string>& out);
};

}
