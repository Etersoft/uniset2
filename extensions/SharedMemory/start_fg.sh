#!/bin/sh

export LD_LIBRARY_PATH="../../lib/.libs;../lib/.libs"

ulimit -Sc 10000000000

./uniset-start.sh -g ./uniset-smemory --smemory-id SharedMemory --pulsar-id DO_C --pulsar-iotype DO \
--confile test.xml --datfile test.xml \
--unideb-add-levels info,crit,warn,level9,system \
--dlog-add-levels info,crit,warn \
--db-logging 1