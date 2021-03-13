#!/bin/sh

ulimit -Sc 1000000

uniset2-start.sh -f ./uniset2-wsgate --confile test.xml --ws-name UWebSocketGate1 --ws-log-add-levels crit,warn,level1 $*
