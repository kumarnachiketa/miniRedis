#!/usr/bin/env bash
# Simple throughput benchmark: send N PINGs and measure requests per second.
# Run ./build/mini_redis in another terminal first.
# Usage: ./scripts/benchmark.sh [N]   (default N=1000)

set -e
cd "$(dirname "$0")/.."
N=${1:-1000}
PORT=${REDIS_PORT:-6379}
HOST=${REDIS_HOST:-localhost}

build_resp() {
    printf $'*1\r\n$4\r\nPING\r\n'
}

echo "Sending $N PING requests to $HOST:$PORT ..."
start=$(python3 -c 'import time; print(time.time())' 2>/dev/null || date +%s.%N)
for ((i=0;i<N;i++)); do
    build_resp | nc -w 2 "$HOST" "$PORT" >/dev/null 2>&1 || true
done
end=$(python3 -c 'import time; print(time.time())' 2>/dev/null || date +%s.%N)
elapsed=1
if command -v python3 &>/dev/null; then
    elapsed=$(python3 -c "print(round($end - $start, 3))")
fi
[ -z "$elapsed" ] || [ "$elapsed" = "0" ] && elapsed=1
rps=$(python3 -c "print(int($N / $elapsed))" 2>/dev/null || echo "$N")
echo "Done: $N requests in ${elapsed}s -> ~$rps req/s"
echo "Tip: set aof_fsync=no in config/server.conf for higher throughput."
