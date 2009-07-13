#!/bin/sh

#ulimit -Sc 1000000000000

uniset-start.sh -f ./uniset-iocontrol --smemory-id SharedMemory \
--confile test.xml \
	 --io-name IOControl \
	 --io-polltime 100 \
	 --io-s-filter-field io \
	 --io-s-filter-value 1 \
	 --iodev1 /dev/null \
	 --iodev2 /dev/null \
	 --io-test-lamp RS_Test9_S \
	 --io-heartbeat-id AI_AS \
	 --io-sm-ready-test-sid RS_Test9_S \
	--unideb-add-levels info,crit,warn,level9,system \

