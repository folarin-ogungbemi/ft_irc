#!/usr/bin/env bash

# ft_irc valgrind test - launches the server under valgrind, then runs a slowed-down stress test.
# Usage:
# chmod +x ValgrindTest.sh
# ./ValgrindTest.sh <server_binary> <port> <password> [clients] [messages_per_client]

set -u

SERV_BIN=${1:?usage: $0 <server_binary> <port> <password> [clients] [msgs]}
PORT=${2:?}
PASS=${3:?}
CLIENTS=${4:-5}
MSGS=${5:-5}

LOG=$(mktemp)
echo "[*] Starting server under valgrind (log: $LOG)"

valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes \
         --errors-for-leak-kinds=definite --error-exitcode=42 \
         --log-file="$LOG" \
         "$SERV_BIN" "$PORT" "$PASS" &
VG_PID=$!
trap 'kill -INT $VG_PID 2>/dev/null; wait $VG_PID 2>/dev/null; rm -f "$LOG"' EXIT

for i in $(seq 1 30); do
    ss -tln | grep -q ":$PORT " && break
    sleep 1
done
ss -tln | grep -q ":$PORT " || { echo "[!] server never started"; exit 1; }

sleep 2
MSGS_SLOW=$((MSGS))
export STRESS_SLEEP=4
./StressTest.sh "$PORT" "$PASS" "$CLIENTS" "$MSGS_SLOW"

kill -INT $VG_PID
wait $VG_PID
VG_EXIT=$?

echo
echo "=== Valgrind summary ==="
grep -E 'definitely lost|indirectly lost|possibly lost|still reachable|ERROR SUMMARY|FILE DESCRIPTORS' "$LOG" || cat "$LOG"

if [ "$VG_EXIT" -eq 42 ]; then
    echo "[!] DEFINITE LEAKS DETECTED"
    exit 1
fi
echo "[*] Done."
