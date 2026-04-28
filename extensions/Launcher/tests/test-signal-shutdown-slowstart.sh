#!/bin/bash
# Регрессионный тест: launcher должен корректно завершаться по SIGINT
# в момент, когда он сидит в waitForReady (процесс ещё не стартовал
# до ready-состояния, readyTimeout очень длинный — 60с).
#
# Покрывает HealthChecker::waitForReady(check, timeout, cancelFlag) —
# чтение cancelFlag в 500мс-чанках во время sleep.

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LAUNCHER_DIR="$(dirname "$SCRIPT_DIR")"
LOG_FILE="/tmp/launcher-signal-shutdown-slowstart-test.log"
SHUTDOWN_TIMEOUT_SEC=5
PRE_SIGNAL_DELAY_SEC=2
MARKER_FILE="/tmp/launcher-slowstart-never-ready.marker"

. "$SCRIPT_DIR/launcher-test-env.sh"
LAUNCHER="$(resolve_launcher "$LAUNCHER_DIR")" || exit 1

LAUNCHER_PID=""

cleanup() {
    if [ -n "$LAUNCHER_PID" ] && kill -0 "$LAUNCHER_PID" 2>/dev/null; then
        kill -KILL "$LAUNCHER_PID" 2>/dev/null || true
    fi
    pkill -f "slowstart-daemon.sh" 2>/dev/null || true
    rm -f "$MARKER_FILE"
}
trap cleanup EXIT

cd "$SCRIPT_DIR"
rm -f "$MARKER_FILE"

echo "[test] Starting launcher with slow-start process (60s readyTimeout)..."
"$LAUNCHER" --confile test-signal-shutdown-slowstart.xml --launcher-name Launcher \
            --http-port 0 > "$LOG_FILE" 2>&1 &
LAUNCHER_PID=$!
echo "[test] Launcher PID: $LAUNCHER_PID"

# launcher должен войти в waitForReady и сидеть там до timeout (60c)
sleep "$PRE_SIGNAL_DELAY_SEC"

if ! kill -0 "$LAUNCHER_PID" 2>/dev/null; then
    echo "[FAIL] Launcher exited prematurely before SIGINT was sent"
    cat "$LOG_FILE"
    exit 1
fi

echo "[test] Sending SIGINT to launcher (PID $LAUNCHER_PID) while waitForReady is active..."
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

echo "[FAIL] Launcher did not exit within ${SHUTDOWN_TIMEOUT_SEC}s after SIGINT (slow-start scenario)"
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
