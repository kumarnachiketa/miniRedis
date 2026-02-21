#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

constexpr int DEFAULT_PORT = 6379;
constexpr const char* DEFAULT_HOST = "127.0.0.1";

int connect_to(const char* host, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        close(fd);
        return -1;
    }
    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

std::string resp_array(const std::vector<std::string>& args) {
    std::string r = "*" + std::to_string(args.size()) + "\r\n";
    for (const auto& a : args) {
        r += "$" + std::to_string(a.size()) + "\r\n" + a + "\r\n";
    }
    return r;
}

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> out;
    std::istringstream iss(line);
    std::string s;
    while (iss >> s) out.push_back(s);
    return out;
}

bool read_line(int fd, std::string& out) {
    out.clear();
    char c;
    while (recv(fd, &c, 1, 0) == 1) {
        if (c == '\r') continue;
        if (c == '\n') return true;
        out += c;
    }
    return false;
}

void read_response(int fd, std::ostream& out) {
    char type;
    if (recv(fd, &type, 1, 0) != 1) return;
    if (type == '+') {
        std::string line;
        if (read_line(fd, line)) out << line << "\n";
        return;
    }
    if (type == '-') {
        std::string line;
        if (read_line(fd, line)) out << "(error) " << line << "\n";
        return;
    }
    if (type == ':') {
        std::string line;
        if (read_line(fd, line)) out << "(integer) " << line << "\n";
        return;
    }
    if (type == '$') {
        std::string len_str;
        if (!read_line(fd, len_str)) return;
        int len = std::stoi(len_str);
        if (len == -1) {
            out << "(nil)\n";
            return;
        }
        std::string bulk(len, '\0');
        size_t total = 0;
        while (total < static_cast<size_t>(len)) {
            ssize_t n = recv(fd, &bulk[total], len - total, 0);
            if (n <= 0) return;
            total += n;
        }
        if (recv(fd, &type, 1, 0) == 1 && type == '\r')
            recv(fd, &type, 1, 0);
        out << "\"" << bulk << "\"\n";
        return;
    }
    if (type == '*') {
        std::string len_str;
        if (!read_line(fd, len_str)) return;
        int n = std::stoi(len_str);
        for (int i = 0; i < n; ++i)
            read_response(fd, out);
        return;
    }
}

int main(int argc, char** argv) {
    const char* host = DEFAULT_HOST;
    int port = DEFAULT_PORT;
    if (argc >= 2) host = argv[1];
    if (argc >= 3) port = std::atoi(argv[2]);

    int fd = connect_to(host, port);
    if (fd < 0) {
        std::cerr << "Could not connect to " << host << ":" << port << "\n";
        return 1;
    }

    std::cout << "miniRedis CLI connected to " << host << ":" << port << "\n";
    std::cout << "Type commands (e.g. PING, SET foo bar, GET foo). Empty line or quit to exit.\n\n";

    std::string line;
    while (std::cout << "> " && std::getline(std::cin, line)) {
        if (line.empty()) break;
        std::string lower = line;
        for (auto& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (lower == "quit" || lower == "exit") break;

        std::vector<std::string> args = tokenize(line);
        if (args.empty()) continue;

        std::string req = resp_array(args);
        if (send(fd, req.data(), req.size(), 0) != static_cast<ssize_t>(req.size())) {
            std::cerr << "Send failed\n";
            break;
        }
        read_response(fd, std::cout);
    }

    close(fd);
    return 0;
}
