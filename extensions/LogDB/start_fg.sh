#!/bin/sh

ulimit -Sc 1000000

#uniset2-start.sh -g \
./uniset2-logdb --confile test.xml --logdb-name LogDB \
 --logdb-log-add-levels any \
 --logdb-dbfile ./test.db \
 --logdb-db-buffer-size 5 \
 --logdb-httpserver-port 8888 \
 --logdb-db-max-records 20000 \
 --logdb-run-logserver 1 \
 --logdb-httpserver-logcontrol-enable \
 $*
