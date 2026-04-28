#!/bin/bash
# Регрессия: процесс становится ready, затем падает; maxRestarts=2 должен
# ограничить общее число рестартов.
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LAUNCHER_DIR="$(dirname "$SCRIPT_DIR")"
LOG_FILE="/tmp/launcher-max-restarts-test.log"
MARKER=/tmp/runtime-crash-ready.marker

. "$SCRIPT_DIR/launcher-test-env.sh"
LAUNCHER="$(resolve_launcher "$LAUNCHER_DIR")" || exit 1

LAUNCHER_PID=""
cleanup() {
    [ -n "$LAUNCHER_PID" ] && kill -KILL "$LAUNCHER_PID" 2>/dev/null || true
    pkill -f "runtime-crash-daemon.sh" 2>/dev/null || true
    rm -f "$MARKER"
}
trap cleanup EXIT

cd "$SCRIPT_DIR"
rm -f "$MARKER"

"$LAUNCHER" --confile test-max-restarts.xml --launcher-name Launcher \
            --http-port 0 --health-interval 200 > "$LOG_FILE" 2>&1 &
LAUNCHER_PID=$!

# Wait long enough for: 1st start (1s) + 2 restarts (1s + delay each) + give-up
# poll up to 12 seconds for "give-up" message
GAVE_UP=0
for i in $(seq 1 120); do
    if grep -qE "exceeded max restarts.*giving up" "$LOG_FILE"; then
        GAVE_UP=1
        break
    fi
    sleep 0.1
done

kill -INT "$LAUNCHER_PID" 2>/dev/null || true
wait "$LAUNCHER_PID" 2>/dev/null || true

# maxRestarts=2 → at most 3 starts (1 initial + 2 restarts).
# Count how many times the daemon was started.
STARTS=$(grep -cE "Starting process: RuntimeCrashProc" "$LOG_FILE" || true)

if [ "$GAVE_UP" -eq 1 ] && [ "$STARTS" -eq 3 ]; then
    echo "[PASS] maxRestarts honored after $STARTS starts"
    exit 0
fi

echo "[FAIL] maxRestarts not honored: $STARTS start attempts seen, gave_up=$GAVE_UP"
echo "--- log tail ---"
tail -50 "$LOG_FILE"
exit 1
