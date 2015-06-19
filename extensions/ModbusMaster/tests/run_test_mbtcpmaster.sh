#!/bin/sh

# '--' - нужен для отделения аргументов catch, от наших..
cd ../../../Utilities/Admin/
./uniset2-start.sh -f ./create_links.sh
./uniset2-start.sh -f ./create

./uniset2-start.sh -f ./exist | grep -q UNISET_PLC/Controllers || exit 1
cd -

./uniset2-start.sh -f ./run_test_mbtcpmaster $* -- --confile mbmaster-test-configure.xml --e-startup-pause 10 \
--mbtcp-name MBTCPMaster1 \
--smemory-id SharedMemory \
--mbtcp-filter-field mb \
--mbtcp-filter-value 2 \
--mbtcp-gateway-iaddr localhost \
--mbtcp-gateway-port 20048 \
--mbtcp-polltime 50 --mbtcp-recv-timeout 500
#--mbtcp-log-add-levels any
#--mbtcp-default-mbinit-ok 1
#--dlog-add-levels any

#--mbtcp-force-out 1
#--dlog-add-levels any
