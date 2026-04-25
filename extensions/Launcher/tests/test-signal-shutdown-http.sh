#!/bin/bash
# Регрессионный тест: launcher должен корректно завершаться по SIGINT,
# когда параллельно идут HTTP-запросы (restart / stop-all) к процессу,
# который непрерывно падает и перезапускается.
#
# Покрывает гипотезу: httpServer->stop() мог бы зависнуть на ожидании
# активного handler'а, который синхронно ждёт завершения медленного
# restartProcess внутри ProcessManager.

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LAUNCHER_DIR="$(dirname "$SCRIPT_DIR")"
LOG_FILE="/tmp/launcher-signal-shutdown-http-test.log"
SHUTDOWN_TIMEOUT_SEC=8
PRE_SIGNAL_DELAY_SEC=3
HTTP_PORT=18099

LAUNCHER="$LAUNCHER_DIR/.libs/uniset2-launcher"
[ ! -x "$LAUNCHER" ] && LAUNCHER="$LAUNCHER_DIR/uniset2-launcher"
[ ! -x "$LAUNCHER" ] && { echo "ERROR: launcher binary not found at $LAUNCHER_DIR"; exit 1; }
command -v curl >/dev/null 2>&1 || { echo "SKIP: curl not available"; exit 77; }

LAUNCHER_PID=""
HTTP_LOAD_PID=""

cleanup() {
    if [ -n "$HTTP_LOAD_PID" ] && kill -0 "$HTTP_LOAD_PID" 2>/dev/null; then
        kill "$HTTP_LOAD_PID" 2>/dev/null || true
    fi
    if [ -n "$LAUNCHER_PID" ] && kill -0 "$LAUNCHER_PID" 2>/dev/null; then
        kill -KILL "$LAUNCHER_PID" 2>/dev/null || true
    fi
    pkill -f "crash-daemon.sh" 2>/dev/null || true
}
trap cleanup EXIT

cd "$SCRIPT_DIR"

echo "[test] Starting launcher with infinitely crashing process + HTTP API..."
"$LAUNCHER" --confile test-signal-shutdown.xml --launcher-name Launcher \
            --http-port $HTTP_PORT --health-interval 200 > "$LOG_FILE" 2>&1 &
LAUNCHER_PID=$!
echo "[test] Launcher PID: $LAUNCHER_PID, HTTP port: $HTTP_PORT"

# Ждём, пока поднимется HTTP-сервер
for i in $(seq 1 30); do
    if curl -s --max-time 1 "http://localhost:$HTTP_PORT/api/v2/launcher/health" >/dev/null 2>&1; then
        break
    fi
    sleep 0.1
done

# Параллельная HTTP-нагрузка: безостановочно дёргаем restart/stop-all
http_loader() {
    local i=0
    while true; do
        curl -s -X POST --max-time 1 \
             "http://localhost:$HTTP_PORT/api/v2/launcher/process/CrashProcess/restart" \
             >/dev/null 2>&1 || true
        sleep 0.15
        curl -s --max-time 1 \
             "http://localhost:$HTTP_PORT/api/v2/launcher/status" \
             >/dev/null 2>&1 || true
        sleep 0.05
        i=$((i+1))
        [ $i -gt 1000 ] && break
    done
}

http_loader &
HTTP_LOAD_PID=$!
echo "[test] HTTP load generator PID: $HTTP_LOAD_PID"

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
        echo "[PASS] Launcher exited within $((i * STEP_MS))ms under HTTP load (exit=$EXIT_CODE)"
        exit 0
    fi
    sleep 0.1
done

echo "[FAIL] Launcher did not exit within ${SHUTDOWN_TIMEOUT_SEC}s after SIGINT (HTTP load active)"
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
