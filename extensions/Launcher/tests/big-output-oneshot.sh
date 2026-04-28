#!/bin/bash
# Helper oneshot for test-oneshot-output.sh: emits ~128 KB to stdout, then
# exits 0. Reproduces the pipe-deadlock scenario where the child writes more
# than the pipe buffer (~64 KB on Linux) and would block on write() unless
# the parent drains stdout concurrently.

set -u

# 2048 lines * ~64 bytes = ~128 KB — comfortably above any pipe buffer.
for i in $(seq 1 2048); do
    echo "line-$i some padding XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
done

exit 0
