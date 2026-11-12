#!/usr/bin/env bash

# ft_irc stress test — parallel nc clients flooding a channel
# Usage:
# chmod +x StressTest.sh
# ./StressTest.sh <port> <password> [clients] [messages_per_client]

set -u

PORT=${1:?usage: $0 <port> <password> [clients] [msgs]}
PASS=${2:?usage: $0 <port> <password> [clients] [msgs]}
CLIENTS=${3:-10}
MSGS=${4:-10}

TMPDIR_T=$(mktemp -d)
PIDS=()

cleanup() {
    echo "[*] Cleaning up..."
    kill "${PIDS[@]}" 2>/dev/null
    rm -rf "$TMPDIR_T"
}
trap cleanup EXIT INT TERM

client_loop() {
    local id=$1
    local out="$TMPDIR_T/client_$id.out"

    {
        printf 'PASS %s\r\n' "$PASS"
        printf 'NICK stress_%02d\r\n' "$id"
        printf 'USER stress_%d 0 * :Stress Client %d\r\n' "$id" "$id"
        printf 'JOIN #stress\r\n'
        local i=0
        while (( i < MSGS )); do
            printf 'PRIVMSG #stress :msg %d from client %d\r\n' "$i" "$id"
            i=$((i + 1))
        done
        sleep 2
        printf 'PART #stress\r\nQUIT :done\r\n'
        sleep 1
    } | nc -q 3 127.0.0.1 "$PORT" > "$out" 2>&1
}

echo "[*] Starting $CLIENTS clients x $MSGS messages on port $PORT"
START=$(date +%s)

for c in $(seq 1 "$CLIENTS"); do
    client_loop "$c" &
    PIDS+=($!)
done

wait "${PIDS[@]}"
DUR=$(( $(date +%s) - START ))

echo
echo "[*] Finished in ${DUR}s. Report:"
TOTAL_MSGS=$(( CLIENTS * MSGS ))
RECV_TOTAL=0
FAIL=0

for f in "$TMPDIR_T"/client_*.out; do
    n=$(grep -c 'PRIVMSG #stress' "$f" 2>/dev/null || true)
    RECV_TOTAL=$((RECV_TOTAL + n))
    if ! grep -q '1 ' "$f"; then
        echo "  [!] $(basename "$f"): missing 001 RPL_WELCOME"
        FAIL=$((FAIL + 1))
    fi
done

echo "  Clients registered OK: $((CLIENTS - FAIL)) / $CLIENTS"
echo "  PRIVMSG lines received total: $RECV_TOTAL (expected ~${TOTAL_MSGS} * clients)"
if grep -q . <<< "$(grep -l 'ERROR' "$TMPDIR_T"/client_*.out 2>/dev/null)"; then
    echo "  [!] Some clients got ERROR lines — check those outputs"
fi

if ss -tlnp | grep -q ":$PORT "; then
    echo "  Server still listening on $PORT: yes"
else
    echo "  [!] SERVER NO LONGER LISTENING on $PORT — it crashed!"
    exit 1
fi
