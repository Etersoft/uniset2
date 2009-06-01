#!/bin/sh

uniset-start.sh -f ./uniset-rtuexchange --rs-name RTUExchange --confile test.xml \
	--rs-filter-field rs --rs-filter-value 1 \
	--rs-dev /dev/cbsideA0 \
	--dlog-add-levels info,crit,warn
#,level3

