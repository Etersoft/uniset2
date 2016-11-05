#!/bin/sh

export LD_LIBRARY_PATH="../../lib/.libs;../lib/.libs"

ulimit -Sc 10000000000

./uniset2-start.sh -f ./uniset2-smemory --smemory-id SharedMemory  \
--confile test-lost.xml --datfile test-lost.xml --ulog-add-levels crit,warn,system,level1 \
--sm-log-add-levels any $* --sm-run-logserver --activator-run-httpserver --activate-timeout 320000

#--pulsar-id DO_C --pulsar-iotype DO --pulsar-msec 100

#--ulog-add-levels info,crit,warn,level9,system \
#--dlog-add-levels info,crit,warn \
