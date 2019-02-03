#!/bin/sh

ulimit -Sc 1000000

uniset2-start.sh -f ./uniset2-pgsql-dbserver --confile test.xml --name DBServer1 \
--pgsql-dbserver-buffer-size 100 \
--pgsql-log-add-levels any $*
