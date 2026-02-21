#pragma once

#include <string>

namespace mini_redis {

class StorageEngine;

class AOFReader {
public:
    explicit AOFReader(const std::string& filename);

    void replay(StorageEngine& engine);

private:
    std::string filename_;
};

} // namespace mini_redis
