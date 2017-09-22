#!/bin/sh

ulimit -Sc 1000000

#uniset2-start.sh -g \
./uniset2-logdb --logdb-single-confile logdb-conf.xml --logdb-name LogDB \
 --logdb-log-add-levels any \
 --logdb-dbfile ./test.db \
 --logdb-buffer-size 5 \
 --logdb-httpserver-port 8888 \
 --logdb-max-records 20000 \
 $*
