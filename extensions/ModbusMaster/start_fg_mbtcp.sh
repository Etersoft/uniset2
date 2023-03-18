#!/bin/sh

./uniset2-start.sh -f ./uniset2-mbtcpmaster \
--confile test.xml \
--mbtcp-name MBMaster1 \
--smemory-id SharedMemory \
--mbtcp-set-prop-prefix \
--mbtcp-filter-field rs \
--mbtcp-filter-value 6 \
--mbtcp-gateway-iaddr localhost \
--mbtcp-gateway-port 2048 \
--mbtcp-recv-timeout 900 \
--mbtcp-polltime 200 \
--mbtcp-polltime 200 \
--mbtcp-force-out 1 \
--mbtcp-persistent-connection 1 \
--ulog-add-levels system \
--mbtcp-run-logserver \
--mbtcp-log-add-levels level3,level4,info,warn,crit \
$*

#--mbtcp-log-add-levels level4,level3 \


#--mbtcp-exchange-mode-id MB1_Mode_AS \
#--mbtcp-filter-field mbtcp --mbtcp-filter-value 1
#--mbtcp-set-prop-prefix rs_ \

