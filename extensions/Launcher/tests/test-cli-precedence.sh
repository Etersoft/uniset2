#!/bin/bash
# Регрессия: XML значения не должны молча перетираться CLI default'ом.
# --http-port 0 должен отключать HTTP, даже если в XML был указан порт.
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LAUNCHER_DIR="$(dirname "$SCRIPT_DIR")"
LOG_FILE="/tmp/launcher-cli-prec-test.log"
TMP_XML=/tmp/cli-prec.xml

. "$SCRIPT_DIR/launcher-test-env.sh"
LAUNCHER="$(resolve_launcher "$LAUNCHER_DIR")" || exit 1

LAUNCHER_PID=""
cleanup() {
    [ -n "$LAUNCHER_PID" ] && kill -KILL "$LAUNCHER_PID" 2>/dev/null || true
    pkill -f "crash-daemon.sh" 2>/dev/null || true
    rm -f "$TMP_XML"
}
trap cleanup EXIT

cd "$SCRIPT_DIR"

# Build a config with httpPort=18099 and healthCheckInterval=200 in XML.
# We will pass --http-port 0 (must disable HTTP) and NO --health-interval
# (must keep XML's 200ms, not CLI default 5000).
sed -e 's|httpPort="0"|httpPort="18099"|' \
    -e 's|healthCheckInterval="[0-9]*"|healthCheckInterval="200"|' \
    test-signal-shutdown.xml > "$TMP_XML"

"$LAUNCHER" --confile "$TMP_XML" --launcher-name Launcher \
            --http-port 0 > "$LOG_FILE" 2>&1 &
LAUNCHER_PID=$!

sleep 2

# Check 1: HTTP must NOT be listening on 18099 (--http-port 0 disabled it)
if curl -s --max-time 1 "http://127.0.0.1:18099/api/v2/launcher/status" >/dev/null 2>&1; then
    echo "[FAIL] --http-port 0 did not disable HTTP (XML port 18099 still serving)"
    kill -INT "$LAUNCHER_PID" 2>/dev/null
    exit 1
fi

# Check 2: log must show 200ms health interval (from XML), NOT 5000ms (CLI default)
if grep -qE "interval[^0-9]*5000" "$LOG_FILE"; then
    echo "[FAIL] XML healthCheckInterval=200 was overridden by CLI default 5000"
    grep -iE "interval" "$LOG_FILE" | head -10
    kill -INT "$LAUNCHER_PID" 2>/dev/null
    exit 1
fi

if ! grep -qE "interval[^0-9]*200" "$LOG_FILE"; then
    echo "[FAIL] expected XML healthCheckInterval=200 not in log"
    grep -iE "interval" "$LOG_FILE" | head -10
    kill -INT "$LAUNCHER_PID" 2>/dev/null
    exit 1
fi

kill -INT "$LAUNCHER_PID" 2>/dev/null
wait "$LAUNCHER_PID" 2>/dev/null || true
echo "[PASS] CLI/XML precedence honored"
