#!/bin/sh

uniset-start.sh -f ./uniset-rtuexchange --rs-name RTUExchange --confile test.xml \
	--rs-filter-field rs --rs-filter-value wago \
	--rs-dev /dev/ttyUSB0 \
	--dlog-add-levels info,crit,warn
#,level3

