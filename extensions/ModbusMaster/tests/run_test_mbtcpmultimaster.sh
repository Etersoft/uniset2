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
--mbtcp-polltime 50 --mbtcp-recv-timeout 500 --mbtcp-timeout 3000 --mbtcp-ignore-timeout 3000 --dlog-add-levels warn,crit
# --dlog-add-levels any
#--mbtcp-force-out 1
#--dlog-add-levels any
