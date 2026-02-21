#include "protocol/parser.hpp"
#include <cstdlib>

namespace protocol {

bool RespParser::parse_array(
        const std::string& in,
        size_t& pos,
        std::vector<std::string>& out)
{
    size_t start = pos;

    if (pos >= in.size() || in[pos] != '*')
        return false;

    pos++;

    size_t end = in.find("\r\n", pos);
    if (end == std::string::npos)
        return false;

    int count = std::stoi(in.substr(pos, end - pos));
    pos = end + 2;

    for (int i = 0; i < count; ++i) {

        if (pos >= in.size() || in[pos] != '$') {
            pos = start;
            return false;
        }

        pos++;

        end = in.find("\r\n", pos);
        if (end == std::string::npos) {
            pos = start;
            return false;
        }

        int len = std::stoi(in.substr(pos, end - pos));
        pos = end + 2;

        if (pos + len + 2 > in.size()) {
            pos = start;
            return false;
        }

        out.push_back(in.substr(pos, len));

        pos += len + 2;
    }

    return true;
}

}

