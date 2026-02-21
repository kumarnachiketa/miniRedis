cmake_minimum_required(VERSION 3.14)
project(mini_redis CXX)

set(CMAKE_CXX_STANDARD 20)

set(SOURCES
  src/main.cpp
  src/common/config.cpp
  src/concurrency/rw_lock.cpp
  src/concurrency/thread_pool.cpp
  src/metrics/exporter.cpp
  src/metrics/metrics.cpp
  src/net/connection.cpp
  src/net/event_loop.cpp
  src/net/server.cpp
  src/net/socket.cpp
  src/persistence/aof_reader.cpp
  src/persistence/aof_writer.cpp
  src/persistence/persistence.cpp
  src/protocol/command.cpp
  src/protocol/parser.cpp
  src/protocol/response.cpp
  src/storage/shard.cpp
  src/storage/storage_engine.cpp
  src/storage/ttl_manager.cpp
)

add_executable(mini_redis ${SOURCES})
target_include_directories(mini_redis PRIVATE include)
target_link_libraries(mini_redis PRIVATE pthread)

# Benchmark client
add_executable(benchmark_client tests/stress/benchmark.cpp)
target_link_libraries(benchmark_client PRIVATE pthread)
