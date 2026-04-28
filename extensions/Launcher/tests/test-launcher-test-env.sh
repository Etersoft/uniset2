#!/bin/sh

set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
. "$SCRIPT_DIR/launcher-test-env.sh"

TMP_DIR="${TMPDIR:-/tmp}/launcher-test-env-$$"
trap 'rm -rf "$TMP_DIR"' EXIT

mkdir -p "$TMP_DIR/.libs"

printf '#!/bin/sh\nexit 0\n' > "$TMP_DIR/uniset2-launcher"
printf '#!/bin/sh\nexit 0\n' > "$TMP_DIR/.libs/uniset2-launcher"
chmod +x "$TMP_DIR/uniset2-launcher" "$TMP_DIR/.libs/uniset2-launcher"

LAUNCHER="$(resolve_launcher "$TMP_DIR")" || exit 1
if [ "$LAUNCHER" != "$TMP_DIR/uniset2-launcher" ]; then
    echo "FAIL: expected libtool wrapper, got: $LAUNCHER"
    exit 1
fi

rm -f "$TMP_DIR/uniset2-launcher"
LAUNCHER="$(resolve_launcher "$TMP_DIR")" || exit 1
if [ "$LAUNCHER" != "$TMP_DIR/.libs/uniset2-launcher" ]; then
    echo "FAIL: expected .libs fallback, got: $LAUNCHER"
    exit 1
fi

rm -f "$TMP_DIR/.libs/uniset2-launcher"
if resolve_launcher "$TMP_DIR" >/dev/null 2>&1; then
    echo "FAIL: missing launcher should fail"
    exit 1
fi

echo "PASS: launcher resolver prefers libtool wrapper"
