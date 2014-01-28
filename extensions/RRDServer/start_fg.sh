#!/bin/sh

uniset-start.sh -f ./uniset-rrdstorage --confile test.xml \
	--rrd-name RRDStorage1 \
	--dlog-add-levels info,crit,warn
	
