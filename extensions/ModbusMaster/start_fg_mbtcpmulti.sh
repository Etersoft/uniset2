#!/bin/sh

./uniset2-start.sh -f ./uniset2-mbtcpmultimaster \
--confile test.xml \
--mbtcp-name MBMultiMaster1 \
--smemory-id SharedMemory \
--dlog-add-levels crit,warn,level4,level9 \
--mbtcp-set-prop-prefix \
--mbtcp-filter-field rs \
--mbtcp-filter-value 5 \
--mbtcp-recv-timeout 500 \
--mbtcp-polltime 200 \
--mbtcp-force-out 1 \
--mbtcp-log-add-levels level4,warn,crit \
--mbtcp-persistent-connection 1 \
--mbtcp-run-logserver \
$*

#--dlog-add-levels info,crit,warn,level4,level3,level9 \
#--mbtcp-exchange-mode-id MB1_Mode_AS \
#--mbtcp-filter-field mbtcp --mbtcp-filter-value 1
#--mbtcp-set-prop-prefix rs_ \

