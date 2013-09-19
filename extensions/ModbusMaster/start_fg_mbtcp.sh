#!/bin/sh

./uniset-start.sh -f ./uniset-mbtcpmaster \
--confile test.xml \
--mbtcp-name MBMaster1 \
--smemory-id SharedMemory \
--dlog-add-levels info,crit,warn,level4,level3 \
--mbtcp-set-prop-prefix \
--mbtcp-filter-field rs \
--mbtcp-filter-value 5 \
--mbtcp-gateway-iaddr 127.0.0.1 \
--mbtcp-gateway-port 2048 \
--mbtcp-recv-timeout 5000 \
--mbtcp-force-disconnect 1 \
--mbtcp-polltime 3000 \
#--mbtcp-set-prop-prefix tcp_
--mbtcp-force-out 1 \
$*

#--mbtcp-exchange-mode-id MB1_Mode_AS \
#--mbtcp-filter-field mbtcp --mbtcp-filter-value 1
#--mbtcp-set-prop-prefix rs_ \

