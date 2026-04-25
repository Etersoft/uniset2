#!/bin/bash
# Регрессионный тест: launcher должен корректно завершаться по SIGINT,
# даже если подопечный процесс бесконечно падает и перезапускается.

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LAUNCHER_DIR="$(dirname "$SCRIPT_DIR")"
LOG_FILE="/tmp/launcher-signal-shutdown-test.log"
SHUTDOWN_TIMEOUT_SEC=5
PRE_SIGNAL_DELAY_SEC=3

LAUNCHER="$LAUNCHER_DIR/.libs/uniset2-launcher"
[ ! -x "$LAUNCHER" ] && LAUNCHER="$LAUNCHER_DIR/uniset2-launcher"
[ ! -x "$LAUNCHER" ] && { echo "ERROR: launcher binary not found at $LAUNCHER_DIR"; exit 1; }

LAUNCHER_PID=""

cleanup() {
    if [ -n "$LAUNCHER_PID" ] && kill -0 "$LAUNCHER_PID" 2>/dev/null; then
        kill -KILL "$LAUNCHER_PID" 2>/dev/null || true
    fi
    pkill -f "crash-daemon.sh" 2>/dev/null || true
}
trap cleanup EXIT

cd "$SCRIPT_DIR"

echo "[test] Starting launcher with infinitely crashing process..."
"$LAUNCHER" --confile test-signal-shutdown.xml --launcher-name Launcher \
            --http-port 0 --health-interval 200 > "$LOG_FILE" 2>&1 &
LAUNCHER_PID=$!
echo "[test] Launcher PID: $LAUNCHER_PID"

sleep "$PRE_SIGNAL_DELAY_SEC"

if ! kill -0 "$LAUNCHER_PID" 2>/dev/null; then
    echo "[FAIL] Launcher exited prematurely before SIGINT was sent"
    cat "$LOG_FILE"
    exit 1
fi

echo "[test] Sending SIGINT to launcher (PID $LAUNCHER_PID)..."
kill -INT "$LAUNCHER_PID"

STEP_MS=100
MAX_ITER=$(( SHUTDOWN_TIMEOUT_SEC * 1000 / STEP_MS ))
for i in $(seq 1 "$MAX_ITER"); do
    if ! kill -0 "$LAUNCHER_PID" 2>/dev/null; then
        wait "$LAUNCHER_PID" 2>/dev/null
        EXIT_CODE=$?
        echo "[PASS] Launcher exited within $((i * STEP_MS))ms (exit=$EXIT_CODE)"
        exit 0
    fi
    sleep 0.1
done

echo "[FAIL] Launcher did not exit within ${SHUTDOWN_TIMEOUT_SEC}s after SIGINT"
echo "--- launcher log: ---"
cat "$LOG_FILE" || true
echo "--- pstack: ---"
if command -v pstack >/dev/null 2>&1; then
    pstack "$LAUNCHER_PID" || true
else
    echo "(pstack not available)"
fi
kill -KILL "$LAUNCHER_PID" 2>/dev/null || true
exit 1
