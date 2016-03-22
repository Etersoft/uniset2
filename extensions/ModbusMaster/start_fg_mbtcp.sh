#!/bin/sh

./uniset2-start.sh -f ./uniset2-mbtcpmaster \
--confile test.xml \
--mbtcp-name MBMaster1 \
--smemory-id SharedMemory \
--mbtcp-log-add-levels level4 \
--mbtcp-set-prop-prefix \
--mbtcp-filter-field rs \
--mbtcp-filter-value 5 \
--mbtcp-gateway-iaddr localhost \
--mbtcp-gateway-port 2048 \
--mbtcp-recv-timeout 900 \
--mbtcp-polltime 200 \
--mbtcp-polltime 200 \
--mbtcp-force-disconnect 1 \
--mbtcp-force-out 1 \
--ulog-add-levels system \
--mbtcp-run-logserver \
$*

#--mbtcp-exchange-mode-id MB1_Mode_AS \
#--mbtcp-filter-field mbtcp --mbtcp-filter-value 1
#--mbtcp-set-prop-prefix rs_ \

