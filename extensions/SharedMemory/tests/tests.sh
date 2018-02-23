#!/bin/sh

./uniset2-start.sh -f ./uniset2-admin --confile ./sm-configure.xml --create
./uniset2-start.sh -f ./uniset2-admin --confile ./sm-configure.xml --exist | grep -q UNISET_SM/Controllers || exit 1

./uniset2-start.sh -f ./uniset2-admin --confile ./reserv-sm-configure.xml --create
./uniset2-start.sh -f ./uniset2-admin --confile ./reserv-sm-configure.xml --exist | grep -q UNISET_SM/Controllers || exit 1


./uniset2-start.sh -f ./tests $* -- --confile ./sm-configure.xml --pulsar-id Pulsar_S --pulsar-msec 1000 --e-filter evnt_test \
--heartbeat-node localhost --heartbeat-check-time 1000 --TestObject-startup-timeout 0 --uniset-object-size-message-queue 2000000
#--ulog-add-levels crit,warn --dlog-add-levels any
#--sm-log-add-levels any --ulog-add-levels level4,warn,crit \
#--TestObject-log-add-levels any
#--dlog-add-levels any
