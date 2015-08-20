#!/bin/sh

./uniset2-start.sh -f ./uniset2-mbtcpmultimaster \
--confile test.xml \
--mbtcp-name MBMultiMaster1 \
--smemory-id SharedMemory \
--dlog-add-levels info,crit,warn,level4,level3,level9 \
--mbtcp-set-prop-prefix \
--mbtcp-filter-field rs \
--mbtcp-filter-value 5 \
--mbtcp-recv-timeout 7000 \
--mbtcp-timeout 2000 \
--mbtcp-polltime 5000 \
--mbtcp-force-out 1 \
--mbtcp-log-add-levels any \
$*

#--mbtcp-exchange-mode-id MB1_Mode_AS \
#--mbtcp-filter-field mbtcp --mbtcp-filter-value 1
#--mbtcp-set-prop-prefix rs_ \

