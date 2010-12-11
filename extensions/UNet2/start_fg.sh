#!/bin/sh

uniset-start.sh -f ./uniset-udpexchange --udp-name UDPExchange --udp-host 192.168.56.255 \
	--udp-broadcast 1 --udp-polltime 1000 \
	--confile test.xml \
	--dlog-add-levels info,crit,warn
#	--udp-filter-field udp --udp-filter-value 1 \
