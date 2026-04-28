#!/bin/bash
# Регрессионный тест: launcher должен правильно репортить exit-код упавшего процесса.
set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LAUNCHER_DIR="$(dirname "$SCRIPT_DIR")"
LOG_FILE="/tmp/launcher-exit-code-test.log"

. "$SCRIPT_DIR/launcher-test-env.sh"
LAUNCHER="$(resolve_launcher "$LAUNCHER_DIR")" || exit 1

LAUNCHER_PID=""
cleanup() {
    [ -n "$LAUNCHER_PID" ] && kill -KILL "$LAUNCHER_PID" 2>/dev/null || true
    pkill -f "crash-daemon.sh" 2>/dev/null || true
}
trap cleanup EXIT

cd "$SCRIPT_DIR"

"$LAUNCHER" --confile test-signal-shutdown.xml --launcher-name Launcher \
            --http-port 0 --health-interval 200 > "$LOG_FILE" 2>&1 &
LAUNCHER_PID=$!

# Poll until launcher logs a real exit code (max 5s)
for i in $(seq 1 50); do
    if grep -qE "CrashProcess \(PID [0-9]+\) exited with code [0-9]+" "$LOG_FILE"; then
        break
    fi
    sleep 0.1
done

kill -INT "$LAUNCHER_PID"
wait "$LAUNCHER_PID" 2>/dev/null || true

# Test: launcher must log real exit code 1 (from crash-daemon.sh "exit 1"),
# not 0 which is what SIGCHLD=SIG_IGN gives us.
if grep -qE "CrashProcess \(PID [0-9]+\) exited with code 1" "$LOG_FILE"; then
    echo "[PASS] real exit code 1 logged"
    exit 0
fi

echo "[FAIL] real exit code 1 not logged"
echo "--- log: ---"
grep -E "CrashProcess|exit" "$LOG_FILE" | head -20
exit 1
