#!/bin/bash
# Регрессия: аргументы после `--` должны приходить в child отдельными
# argv-элементами, а не одной слепленной строкой.
set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LAUNCHER_DIR="$(dirname "$SCRIPT_DIR")"
LOG_FILE="/tmp/launcher-passthrough-test.log"

. "$SCRIPT_DIR/launcher-test-env.sh"
LAUNCHER="$(resolve_launcher "$LAUNCHER_DIR")" || exit 1

LAUNCHER_PID=""
cleanup() {
    [ -n "$LAUNCHER_PID" ] && kill -KILL "$LAUNCHER_PID" 2>/dev/null || true
    pkill -f "echo-args-daemon.sh" 2>/dev/null || true
}
trap cleanup EXIT

cd "$SCRIPT_DIR"

"$LAUNCHER" --confile test-passthrough-args.xml --launcher-name Launcher \
            --http-port 0 -- --custom-arg value > "$LOG_FILE" 2>&1 &
LAUNCHER_PID=$!

# Wait for child to print its argv (poll up to 5s)
SAW_CUSTOM_ARG=0
SAW_VALUE=0
for i in $(seq 1 50); do
    if grep -qE 'argv\[[0-9]+\]=--custom-arg' "$LOG_FILE"; then
        SAW_CUSTOM_ARG=1
    fi
    if grep -qE 'argv\[[0-9]+\]=value' "$LOG_FILE"; then
        SAW_VALUE=1
    fi
    if [ "$SAW_CUSTOM_ARG" -eq 1 ] && [ "$SAW_VALUE" -eq 1 ]; then
        break
    fi
    sleep 0.1
done

kill -INT "$LAUNCHER_PID" 2>/dev/null || true
wait "$LAUNCHER_PID" 2>/dev/null || true

if [ "$SAW_CUSTOM_ARG" -eq 1 ] && [ "$SAW_VALUE" -eq 1 ]; then
    # Also check NO single argv contains both joined together
    if grep -qE 'argv\[[0-9]+\]=--custom-arg value' "$LOG_FILE"; then
        echo "[FAIL] args concatenated into single argv element"
        grep "argv\[" "$LOG_FILE" | head -10
        exit 1
    fi
    echo "[PASS] passthrough args delivered as separate argv elements"
    exit 0
fi

echo "[FAIL] expected separate argv[N]=--custom-arg AND argv[N]=value not found"
echo "--- daemon argv ---"
grep "argv\[" "$LOG_FILE" | head -10
echo "--- log tail ---"
tail -20 "$LOG_FILE"
exit 1
