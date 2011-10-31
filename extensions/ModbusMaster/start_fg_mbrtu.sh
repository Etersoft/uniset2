#!/bin/sh

uniset-start.sh -f ./uniset-rtuexchange --confile test.xml \
	--smemory-id SharedMemory \
	--rs-dev /dev/cbsideA0 \
	--rs-name RTUExchange \
	--rs-speed 38400 \
	--rs-filter-field rs \
	--rs-filter-value 1 \
	--dlog-add-levels info,crit,warn,level4,level3 \
	--rs-force 0 \
	--rs-force-out 0 \
	--rs-polltime 3000
#,level3
#	--rs-force 1 \
