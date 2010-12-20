#!/bin/sh

uniset-start.sh -f ./uniset-udpexchange --udp-name UDPExchange \
	--confile test.xml \
	--udp-filter-field udp --udp-filter-value 1 \
	--dlog-add-levels info,crit,warn

