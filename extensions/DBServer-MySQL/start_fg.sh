#!/bin/sh

ulimit -Sc 1000000

uniset-start.sh -f ./uniset-mysql-dbserver --confile test.xml --name DBServer1 \
--unideb-add-levels info,crit,warn,level9,system \
--dbserver-buffer-size 100

