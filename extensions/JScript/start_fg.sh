#!/bin/sh

uniset2-start.sh -f ./uniset2-jscript --confile test.xml --js-log-add-levels any \
 --js-file main.js \
 --js-name TestProc \
 --js-confnode JSProxy \
 --js-run-logserver 1 \
 --ulog-add-levels any \
 $*
