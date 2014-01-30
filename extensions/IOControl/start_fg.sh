#!/bin/sh

export LD_LIBRARY_PATH="../../lib/.libs;../lib/.libs"
#ulimit -Sc 1000000000000

uniset-start.sh -f ./uniset-iocontrol --smemory-id SharedMemory \
--confile test.xml \
	 --io-name IOControl \
	 --io-polltime 100 \
	 --io-s-filter-field io \
	 --io-s-filter-value 1 \
	 --io-test-lamp Input1_S \
	 --io-heartbeat-id AI_AS \
	 --io-sm-ready-test-sid Input1_S \
	 --unideb-add-levels any --dlog-add-levels any

