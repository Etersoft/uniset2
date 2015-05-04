#!/bin/sh

./uniset2-start.sh -f ./uniset2-admin --confile ./sm-configure.xml --create
./uniset2-start.sh -f ./uniset2-admin --confile ./sm-configure.xml --exist | grep -q UNISET_SM/Controllers || exit 1

./uniset2-start.sh -f ./uniset2-admin --confile ./reserv-sm-configure.xml --create
./uniset2-start.sh -f ./uniset2-admin --confile ./reserv-sm-configure.xml --exist | grep -q UNISET_SM/Controllers || exit 1


./uniset2-start.sh -f ./tests $* -- --confile ./sm-configure.xml --pulsar-id Pulsar_S --pulsar-msec 1000 \
--heartbeat-node localhost --heartbeat-check-time 1000 
#--dlog-add-levels any
