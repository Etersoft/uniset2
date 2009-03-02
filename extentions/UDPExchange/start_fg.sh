#!/bin/sh

uniset-start.sh -f ./uniset-udpexchange --udp-name UDPExchange --udp-host localhost --udp-port 2049 \
	--confile test.xml \
	--udp-filter-field udp --udp-filter-value 1 \
	--udp-ip \
	--dlog-add-levels info,crit,warn

