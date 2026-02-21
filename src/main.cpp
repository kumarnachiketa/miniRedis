#include "storage/storage_engine.hpp"
#include "net/server.hpp"
#include "common/config.hpp"

#include <iostream>

int main() {
    mini_redis::ServerConfig config =
        mini_redis::load_config("config/server.conf");

    mini_redis::StorageEngine engine(config.shard_count);
    engine.enable_aof(config.aof_file);

    net::Server server(config.port, engine);
    if (!server.start()) {
        std::cerr << "Failed to start server\n";
        return 1;
    }

    server.run();
    return 0;
}
