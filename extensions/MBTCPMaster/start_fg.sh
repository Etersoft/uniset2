#!/bin/sh

uniset-start.sh -f ./uniset-mbtcpmaster \
--confile test.xml \
--mbtcp-name MBMaster1 \
--smemory-id SharedMemory \
--dlog-add-levels info,crit,warn,level4 \
--mbtcp-filter-field mbtcp \
--mbtcp-filter-value 1 \
--mbtcp-gateway-iaddr 127.0.0.1 \
--mbtcp-gateway-port 1024 \
--mbtcp-recv-timeout 5000 
#--mbtcp-filter-field mbtcp --mbtcp-filter-value 1

