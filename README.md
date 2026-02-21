# miniRedis

A minimal Redis-like in-memory key-value store in C++. TCP server on port 6379 with RESP protocol, sharded storage, TTL support, and optional AOF persistence.

## Features

- **Commands:** `GET`, `SET`, `SETEX` (set with TTL), `DEL`
- **Sharded storage** — 64 shards by default for concurrent access with per-shard locking
- **TTL** — expiration via `SETEX` and background TTL cleanup
- **Persistence** — optional append-only file (AOF) for durability
- **Protocol** — Redis-compatible RESP (REdis Serialization Protocol)

## Requirements

- C++20 compiler (e.g. GCC 10+, Clang 10+, Apple Clang 14+)
- CMake 3.14+ (optional; see build script for plain `g++`)

## Build

**Using CMake (recommended):**

```bash
cmake -B build -S .
cmake --build build
```

**Using the build script:**

```bash
./scripts/build.sh
```

Binary: `build/mini_redis` (with CMake) or `./mini_redis` (with script fallback).

## Run

```bash
./build/mini_redis

in mac
printf '*1\r\n$4\r\nPING\r\n' | nc localhost 6379
```

Server listens on **port 6379**. AOF is enabled and writes to `aof.log` in the current directory.

**Example with `redis-cli`:**

```bash
redis-cli -p 6379 SET foo bar
redis-cli -p 6379 GET foo
redis-cli -p 6379 SETEX key 10 value
redis-cli -p 6379 DEL foo
```

## Project layout

```
├── CMakeLists.txt
├── config/
│   └── server.yml
├── include/
│   ├── common/       # types, status, hash, time
│   ├── concurrency/  # thread_pool, rw_lock, spin_lock
│   ├── metrics/      # metrics, exporter
│   ├── net/          # server, connection, event_loop, socket
│   ├── persistence/  # aof_writer, aof_reader, snapshot
│   ├── protocol/     # parser, command, response
│   └── storage/      # storage_engine, shard, value, ttl_manager
├── src/              # implementations (mirrors include/)
├── tests/
│   ├── unit/         # test_storage, test_ttl, test_parser
│   ├── integration/  # test_persistence, test_server
│   └── stress/       # benchmark
└── scripts/          # build.sh, run_server.sh, benchmark.sh, cleanup.sh
```

## License

MIT (or as specified in the repo.)
