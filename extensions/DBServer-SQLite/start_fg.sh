#!/bin/sh

ulimit -Sc 1000000

uniset-start.sh -f ./uniset-sqlite-dbserver --confile test.xml --name DBServer1 \
--ulog-add-levels info,crit,warn,level9,system \
--dbserver-buffer-size 100

