#!/bin/sh

uniset-start.sh -f ./uniset-mbtcpmaster --mbtcp-name MBTCPExchange1 \
--dlog-add-levels info,crit,warn \
--mbtcp-filter-field CAN2sender \
--mbtcp-filter-value SYSTSNode \
--mbtcp-gateway-iaddr 127.0.0.1 \
--mbtcp-gateway-port 2048
#--mbtcp-filter-field mbtcp --mbtcp-filter-value 1

