# miniRedis

A minimal Redis-like in-memory key-value store in C++. TCP server on port 6379 with RESP protocol, sharded storage, TTL support, and optional AOF persistence.

## Features

- **Commands:** `PING`, `GET`, `SET`, `SETEX`, `DEL`, `EXISTS`, `EXPIRE`, `TTL`, `KEYS pattern`
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

Binary: `build/mini_redis` (with CMake) or `./mini_redis` (with script fallback). The server uses a **kqueue** event loop and non-blocking I/O for 1000+ req/s; set `aof_fsync=no` in config for maximum throughput.

## Run

```bash
./build/mini_redis
```

Server listens on **port 6379** (configurable via `config/server.conf`). AOF is enabled and writes to `aof.log` by default.

**Configuration:**

Edit `config/server.conf` to change:
- `port` — server port (default: 6379)
- `aof_file` — AOF log path (default: aof.log)
- `shards` — number of storage shards (default: 64)
- `aof_fsync` — `every_write` (default) or `no` (higher throughput, less durable)

**Benchmark:**

**C++ benchmark (recommended - much faster):**
```bash
./scripts/benchmark_cpp.sh 10000 4
# or: ./build/benchmark_client 10000 4
```
Runs 10,000 requests across 4 threads. Make sure server is running first.

**Shell benchmark (slower, for quick tests):**
```bash
./scripts/benchmark.sh 1000
```
Note: The C++ benchmark is 100x+ faster because it reuses connections and uses multiple threads.

**Testing:**

**Option 1: Test script (easiest)**
```bash
./scripts/test.sh PING
./scripts/test.sh SET foo bar
./scripts/test.sh GET foo
./scripts/test.sh SETEX key 10 value
./scripts/test.sh DEL foo
```

**Option 2: redis-cli**

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
│   └── server.conf      # server configuration
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
└── scripts/          # build.sh, test.sh, run_server.sh, benchmark.sh, cleanup.sh
```

## License

MIT (or as specified in the repo.)
