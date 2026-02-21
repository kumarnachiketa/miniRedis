#!/usr/bin/env bash
# Test miniRedis commands via netcat
# Usage: ./scripts/test.sh [command args...]
# Examples:
#   ./scripts/test.sh PING
#   ./scripts/test.sh SET foo bar
#   ./scripts/test.sh GET foo
#   ./scripts/test.sh SETEX key 10 value
#   ./scripts/test.sh DEL foo

PORT=${REDIS_PORT:-6379}
HOST=${REDIS_HOST:-localhost}

# Build RESP array from args. Use $'...' so \r\n are real CRLF in bash.
build_resp() {
    local count=$#
    printf $'*%d\r\n' "$count"
    for arg in "$@"; do
        printf $'$%d\r\n%s\r\n' "${#arg}" "$arg"
    done
}

# Send command and show response. Close stdin after send; -w 2 lets nc wait for reply (macOS).
send_cmd() {
    echo "→ Sending: $*"
    build_resp "$@" | nc -w 2 "$HOST" "$PORT"
    echo ""
}

if [ $# -eq 0 ]; then
    echo "Usage: $0 COMMAND [args...]"
    echo ""
    echo "Examples:"
    echo "  $0 PING"
    echo "  $0 SET foo bar"
    echo "  $0 GET foo"
    echo "  $0 SETEX key 10 value"
    echo "  $0 DEL foo"
    exit 1
fi

send_cmd "$@"
