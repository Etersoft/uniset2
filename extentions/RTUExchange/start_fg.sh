#!/bin/sh

uniset-start.sh -f ./uniset-rtuexchange --confile test.xml \
	--rs-dev /dev/cbsideA0 \
	--rs-name RSExchange \
	--rs-speed 38400 \
	--rs-filter-field rs \
	--rs-filter-value 2 \
	--dlog-add-levels info,crit,warn,level1
#,level3

