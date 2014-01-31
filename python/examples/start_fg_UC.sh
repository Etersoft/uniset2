#!/bin/sh

START=uniset2-start.sh

${START} -f ./testUC.py --ulog-add-levels info,warn,crit
