#!/bin/sh

START=./uniset-start.sh

export LD_LIBRARY_PATH="../../lib/.libs;../../lib/pyUniSet/.libs"

${START} -f ./testUC.py --unideb-add-levels info,warn,crit
