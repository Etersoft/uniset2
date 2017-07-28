#!/bin/sh

START=./uniset2-start.sh

export LD_LIBRARY_PATH="../../lib/.libs;../../lib/pyUniSet/.libs"

${START} -f ./testUC.py --ulog-add-levels info,warn,crit
