#!/bin/sh

export LD_LIBRARY_PATH="../../lib/.libs;../lib/.libs"

ulimit -Sc 1000000000000

./uniset-start.sh -f ./uniset-smemory --smemory-id SharedMemory \
--confile test.xml --datfile test.xml \
--unideb-add-levels info,crit,warn,level9,system \
--dlog-add-levels info,crit,warn

