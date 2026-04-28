#!/bin/bash
# Регрессия: oneshot, выдающий >>64 KB на stdout, должен корректно
# отрабатывать (пайп stdout/stderr дренится в фоновом потоке) и
# не блокировать запуск последующих процессов.
# Покрывает drain-логику в ProcessManager.cc:551-648.
set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LAUNCHER_DIR="$(dirname "$SCRIPT_DIR")"
LOG_FILE="/tmp/launcher-oneshot-output-test.log"

. "$SCRIPT_DIR/launcher-test-env.sh"
LAUNCHER="$(resolve_launcher "$LAUNCHER_DIR")" || exit 1

LAUNCHER_PID=""
cleanup() {
    [ -n "$LAUNCHER_PID" ] && kill -KILL "$LAUNCHER_PID" 2>/dev/null || true
    pkill -f "big-output-oneshot.sh" 2>/dev/null || true
    pkill -f "crash-daemon.sh" 2>/dev/null || true
}
trap cleanup EXIT

cd "$SCRIPT_DIR"

"$LAUNCHER" --confile test-oneshot-output.xml --launcher-name Launcher \
            --http-port 0 > "$LOG_FILE" 2>&1 &
LAUNCHER_PID=$!

# Ждём, пока oneshot отработает. Без drain'а пайпа child заблокируется
# на write() после ~64 KB, и эта строка никогда не появится.
PASS=0
for i in $(seq 1 50); do
    if grep -qE "BigOutputOneshot completed successfully" "$LOG_FILE"; then
        PASS=1
        break
    fi
    if ! kill -0 "$LAUNCHER_PID" 2>/dev/null; then
        break
    fi
    sleep 0.1
done

# Дополнительная проверка: launcher корректно реагирует на SIGTERM
# и завершается в разумное время (не висит в ожидании drain'а).
kill -TERM "$LAUNCHER_PID" 2>/dev/null || true
SHUTDOWN_OK=0
for i in $(seq 1 80); do
    if ! kill -0 "$LAUNCHER_PID" 2>/dev/null; then
        wait "$LAUNCHER_PID" 2>/dev/null
        SHUTDOWN_OK=1
        break
    fi
    sleep 0.1
done

if [ "$SHUTDOWN_OK" -ne 1 ]; then
    echo "[FAIL] launcher did not shut down within 8s after SIGTERM"
    echo "--- log tail ---"
    tail -50 "$LOG_FILE"
    kill -KILL "$LAUNCHER_PID" 2>/dev/null || true
    exit 1
fi

if [ "$PASS" -eq 1 ]; then
    echo "[PASS] oneshot with large output drained without deadlock"
    exit 0
fi

echo "[FAIL] oneshot with large stdout deadlocked - drain logic regressed"
echo "--- log tail ---"
tail -50 "$LOG_FILE"
exit 1
