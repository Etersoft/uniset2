#!/bin/sh

ulimit -Sc 1000000

#uniset2-start.sh -g \
./uniset2-logdb --logdb-single-confile logdb-conf.xml --logdb-name LogDB --logdb-db-disable \
 --logdb-log-add-levels any \
 --logdb-db-dbfile ./test.db \
 --logdb-db-buffer-size 5 \
 --logdb-httpserver-port 8888 \
 --logdb-db-max-records 20000 \
 $*
