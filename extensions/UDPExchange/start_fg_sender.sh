#!/bin/sh

uniset-start.sh -f ./uniset-udpsender --udp-name UDPExchange \
	--udp-host 192.168.56.255 --udp-port 2050 --udp-broadcast 1\
	--udp-sendtime 2000 \
	--confile test.xml \
	--dlog-add-levels info,crit,warn
#	--udp-filter-field udp --udp-filter-value 1 \
