#!/bin/sh

ulimit -Sc 1000000

uniset2-start.sh -f ./uniset2-sqlite-dbserver --confile test.xml --name DBServer1 \
--ulog-add-levels info,crit,warn,level9,system \
--dbserver-buffer-size 100
