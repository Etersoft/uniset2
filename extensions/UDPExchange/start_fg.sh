#!/bin/sh

uniset-start.sh -f ./uniset-udpexchange --udp-name UDPExchange --udp-host 192.168.56.255 --udp-port 2050 --udp-broadcast 1 \
	--confile test.xml \
	--udp-filter-field udp --udp-filter-value 1 \
	--dlog-add-levels info,crit,warn

