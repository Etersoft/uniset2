#!/bin/sh

./uniset2-start.sh -f ./uniset2-admin --confile ./lp-configure.xml --create
./uniset2-start.sh -f ./uniset2-admin --confile ./lp-configure.xml --exist | grep -q UNISET_LP/Controllers || exit 1

./uniset2-start.sh -f ./tests $* -- --confile ./lp-configure.xml --sleepTime 50
#--ulog-add-levels any
#--dlog-add-levels any
