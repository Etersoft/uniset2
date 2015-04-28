#!/bin/sh

./uniset2-start.sh -f ./uniset2-admin --confile ./sm-configure.xml --create
./uniset2-start.sh -f ./uniset2-admin --confile ./sm-configure.xml --exist | grep -q UNISET_SM/Controllers || exit 1

./uniset2-start.sh -f ./uniset2-admin --confile ./reserv-sm-configure.xml --create
./uniset2-start.sh -f ./uniset2-admin --confile ./reserv-sm-configure.xml --exist | grep -q UNISET_SM/Controllers || exit 1


./uniset2-start.sh -f ./tests $* -- --confile ./sm-configure.xml --dlog-add-levels any
