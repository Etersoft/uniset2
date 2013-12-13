#!/bin/sh

uniset-start.sh -f ./uniset-rrdserver --confile test.xml \
	--rrd-name RRDServer1 \
	--dlog-add-levels info,crit,warn
