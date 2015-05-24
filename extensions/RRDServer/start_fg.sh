#!/bin/sh

uniset2-start.sh -f ./uniset2-rrdserver --confile test.xml \
	--rrd-name RRDServer1 \
	--rrd-log-add-levels any $*
