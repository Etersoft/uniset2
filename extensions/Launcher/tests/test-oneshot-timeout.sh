#!/bin/bash
# Регрессия: oneshot, который зависает, должен быть убит по oneshotTimeout
# и не блокировать дальнейший запуск.
set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LAUNCHER_DIR="$(dirname "$SCRIPT_DIR")"
LOG_FILE="/tmp/launcher-oneshot-timeout-test.log"

. "$SCRIPT_DIR/launcher-test-env.sh"
LAUNCHER="$(resolve_launcher "$LAUNCHER_DIR")" || exit 1

LAUNCHER_PID=""
cleanup() {
    [ -n "$LAUNCHER_PID" ] && kill -KILL "$LAUNCHER_PID" 2>/dev/null || true
    pkill -f "hanging-oneshot.sh" 2>/dev/null || true
    pkill -f "crash-daemon.sh" 2>/dev/null || true
}
trap cleanup EXIT

cd "$SCRIPT_DIR"

"$LAUNCHER" --confile test-oneshot-timeout.xml --launcher-name Launcher \
            --http-port 0 > "$LOG_FILE" 2>&1 &
LAUNCHER_PID=$!

# Within 5s, oneshotTimeout=1.5s should fire. Either we see kill log,
# or AfterOneshot starts (which means launcher proceeded past the hang).
PASS=0
for i in $(seq 1 50); do
    if grep -qE "HangingOneshot.*timeout|HangingOneshot.*killed" "$LOG_FILE"; then
        PASS=1
        break
    fi
    sleep 0.1
done

kill -INT "$LAUNCHER_PID" 2>/dev/null || true
wait "$LAUNCHER_PID" 2>/dev/null || true

if [ "$PASS" -eq 1 ]; then
    echo "[PASS] oneshot timeout enforced"
    exit 0
fi

echo "[FAIL] hanging oneshot blocked launcher >5s, timeout not enforced"
echo "--- log tail ---"
tail -30 "$LOG_FILE"
exit 1
