#!/bin/sh

export LD_LIBRARY_PATH="../../lib/.libs;../lib/.libs"

ulimit -Sc 10000000000

./uniset-start.sh -f ./uniset-network --smemory-id SharedMemory \
--unideb-add-levels info,crit,warn,level9,system \
--dlog-add-levels info,crit,warn

