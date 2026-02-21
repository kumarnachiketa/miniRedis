#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <cstring>
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>

constexpr int PORT = 6379;
constexpr const char* HOST = "localhost";

int connect_socket() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, HOST, &addr.sin_addr);
    
    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    
    return fd;  // Keep blocking for benchmark
}

void send_ping(int fd) {
    const char* cmd = "*1\r\n$4\r\nPING\r\n";
    send(fd, cmd, strlen(cmd), 0);
}

bool recv_pong(int fd) {
    char buf[64];
    ssize_t n = recv(fd, buf, sizeof(buf), 0);
    return n > 0;
}

void benchmark_worker(int requests, int& success) {
    int fd = connect_socket();
    if (fd < 0) {
        success = 0;
        return;
    }
    
    int ok = 0;
    for (int i = 0; i < requests; ++i) {
        send_ping(fd);
        if (recv_pong(fd)) ++ok;
    }
    close(fd);
    success = ok;
}

int main(int argc, char** argv) {
    int total_requests = 10000;
    int num_threads = 4;
    
    if (argc > 1) total_requests = std::atoi(argv[1]);
    if (argc > 2) num_threads = std::atoi(argv[2]);
    
    int per_thread = total_requests / num_threads;
    if (per_thread == 0) per_thread = 1;
    
    std::cout << "Benchmark: " << total_requests << " requests, " 
              << num_threads << " threads (" << per_thread << " per thread)\n";
    std::cout << "Make sure server is running with aof_fsync=no\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::thread> threads;
    std::vector<int> results(num_threads);
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(benchmark_worker, per_thread, std::ref(results[i]));
    }
    
    for (auto& t : threads) t.join();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    double elapsed = elapsed_ms / 1000.0;
    
    int total_success = 0;
    for (int r : results) total_success += r;
    
    double rps = total_success / elapsed;
    
    std::cout << "\nResults:\n";
    std::cout << "  Requests: " << total_success << "/" << total_requests << "\n";
    std::cout << "  Time: " << elapsed << "s\n";
    std::cout << "  Throughput: ~" << static_cast<int>(rps) << " req/s\n";
    
    return 0;
}
