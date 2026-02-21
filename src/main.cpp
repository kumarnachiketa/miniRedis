#include "storage/storage_engine.hpp"
#include "net/server.hpp"

#include <iostream>

int main() {

    mini_redis::StorageEngine engine(64);

    engine.enable_aof("aof.log");

    net::Server server(6379, engine);

    if (!server.start()) {
        std::cerr << "Failed to start server\n";
        return 1;
    }

    server.run();

    return 0;
}
