#!/bin/sh

uniset-start.sh -f ./uniset-rtuexchange --confile test.xml \
	--rs-dev /dev/ttyUSB0 \
	--rs-name RSExchange \
	--rs-speed 38400 \
	--rs-filter-field rs \
	--rs-filter-value 1 \
	--dlog-add-levels info,crit,warn
#,level3

