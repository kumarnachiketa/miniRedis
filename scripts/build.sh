#!/usr/bin/env bash
# Build mini_redis (use CMake preferred, or run this script from repo root)
set -e
cd "$(dirname "$0")/.."

if command -v cmake &>/dev/null; then
  cmake -B build -S .
  cmake --build build
  echo "Built: build/mini_redis"
else
  g++ -std=c++20 -pthread \
    src/main.cpp \
    src/concurrency/rw_lock.cpp src/concurrency/thread_pool.cpp \
    src/metrics/exporter.cpp src/metrics/metrics.cpp \
    src/net/connection.cpp src/net/event_loop.cpp src/net/server.cpp src/net/socket.cpp \
    src/persistence/aof_reader.cpp src/persistence/aof_writer.cpp src/persistence/persistence.cpp \
    src/protocol/command.cpp src/protocol/parser.cpp src/protocol/response.cpp \
    src/storage/shard.cpp src/storage/storage_engine.cpp src/storage/ttl_manager.cpp \
    -I include -o mini_redis
  echo "Built: ./mini_redis"
fi
