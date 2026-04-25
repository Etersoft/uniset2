#!/bin/bash
# Регрессионный тест: launcher должен корректно завершаться по SIGINT,
# когда подопечный процесс «не может запуститься» — он живёт, но
# никогда не достигает ready-состояния, и launcher бесконечно kill'ит
# и перезапускает его.
#
# Покрывает retry-loop в ProcessManager::startProcessWithUnlock(),
# крутящийся через interruptibleSleep(stopping_).

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LAUNCHER_DIR="$(dirname "$SCRIPT_DIR")"
LOG_FILE="/tmp/launcher-signal-shutdown-unready-test.log"
SHUTDOWN_TIMEOUT_SEC=8
PRE_SIGNAL_DELAY_SEC=4
MARKER_FILE="/tmp/launcher-never-ready.marker"

LAUNCHER="$LAUNCHER_DIR/.libs/uniset2-launcher"
[ ! -x "$LAUNCHER" ] && LAUNCHER="$LAUNCHER_DIR/uniset2-launcher"
[ ! -x "$LAUNCHER" ] && { echo "ERROR: launcher binary not found at $LAUNCHER_DIR"; exit 1; }

LAUNCHER_PID=""

cleanup() {
    if [ -n "$LAUNCHER_PID" ] && kill -0 "$LAUNCHER_PID" 2>/dev/null; then
        kill -KILL "$LAUNCHER_PID" 2>/dev/null || true
    fi
    pkill -f "unready-daemon.sh" 2>/dev/null || true
    rm -f "$MARKER_FILE"
}
trap cleanup EXIT

cd "$SCRIPT_DIR"
rm -f "$MARKER_FILE"

echo "[test] Starting launcher with unready process (will never reach ready state)..."
"$LAUNCHER" --confile test-signal-shutdown-unready.xml --launcher-name Launcher \
            --http-port 0 > "$LOG_FILE" 2>&1 &
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

echo "[FAIL] Launcher did not exit within ${SHUTDOWN_TIMEOUT_SEC}s after SIGINT (unready-process scenario)"
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
