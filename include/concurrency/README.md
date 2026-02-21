# Concurrency (stubs for future use)

These headers and sources are placeholders for future implementation:

- **thread_pool.hpp / .cpp** — For multi-threaded command execution (e.g. run commands on workers while main thread does I/O).
- **rw_lock.hpp / .cpp** — Optional shared/exclusive lock utilities (storage already uses `std::shared_mutex` per shard).
- **spin_lock.hpp** — Optional low-contention lock for very short critical sections.

The current server uses a **single-threaded kqueue event loop** and is tuned for 1000+ req/s. To scale further, consider:

1. Implementing a thread pool and dispatching command execution to workers.
2. Using `rw_lock` or similar where finer-grained locking is needed.
3. Keeping the event loop on one thread and only offloading CPU-heavy work to the pool.
