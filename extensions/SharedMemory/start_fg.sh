#!/bin/sh

export LD_LIBRARY_PATH="../../lib/.libs;../lib/.libs"

ulimit -Sc 10000000000

./uniset2-start.sh -f ./uniset2-smemory --smemory-id SharedMemory --pulsar-id DO_C --pulsar-iotype DO --pulsar-msec 100 \
--confile test.xml --datfile test.xml --db-logging 1 --ulog-add-levels system \
--sm-log-add-levels any $* \
#--ulog-add-levels info,crit,warn,level9,system \
#--dlog-add-levels info,crit,warn \
