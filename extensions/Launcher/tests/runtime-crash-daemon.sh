#!/bin/bash
# Daemon that becomes ready (touches marker) and then exits with code 1 after 1s.
# NB: launcher forwards extra args (--uniset-port PORT) to children, so we hardcode
# the marker path instead of taking it from $2.
NAME=runtime-crash
MARKER=/tmp/runtime-crash-ready.marker
echo "$NAME: started"
touch "$MARKER"
sleep 1
echo "$NAME: simulating runtime crash"
exit 1
