#!/bin/sh

JSFILE="main-esm.js"
[ -n "$1" ] && JSFILE="$1"

uniset2-start.sh -f ./uniset2-jscript --confile test.xml --js-log-add-levels any \
 --js-file ${JSFILE} \
 --js-esmModuleMode 1 \
 --js-name TestProc \
 --js-confnode JSProxy \
 $*
