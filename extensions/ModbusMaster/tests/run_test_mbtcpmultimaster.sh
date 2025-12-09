#!/bin/sh

# '--' - нужен для отделения аргументов catch, от наших..
cd ../../../Utilities/Admin/
./uniset2-start.sh -f ./create_links.sh
./uniset2-start.sh -f ./create

./uniset2-start.sh -f ./exist | grep -q UNISET_PLC/Controllers || exit 1
cd -

./uniset2-start.sh -f ./run_test_mbtcpmultimaster $* -- --confile mbmaster-test-configure.xml --e-startup-pause 10 \
--mbtcp-name MBTCPMultiMaster1 \
--smemory-id SharedMemory \
--mbtcp-filter-field mb \
--mbtcp-filter-value 1 \
--mbtcp-polltime 50 --mbtcp-recv-timeout 500 --mbtcp-checktime 1000 --mbtcp-timeout 2500 --mbtcp-ignore-timeout 2500 \
--mbtcp-init-pause 1000 \
--dlog-add-levels warn,crit \
--activator-run-httpserver --activator-httpserver-port 9191
# --mbtcp-log-add-levels any
# --dlog-add-levels any
#--mbtcp-force-out 1
#--dlog-add-levels any
