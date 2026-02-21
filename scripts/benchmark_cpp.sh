#!/usr/bin/env bash
# Run the C++ benchmark client (much faster than shell script)
# Usage: ./scripts/benchmark_cpp.sh [requests] [threads]
# Example: ./scripts/benchmark_cpp.sh 10000 4

set -e
cd "$(dirname "$0")/.."

if [ ! -f build/benchmark_client ]; then
    echo "Building benchmark client..."
    cmake --build build --target benchmark_client
fi

./build/benchmark_client "$@"
