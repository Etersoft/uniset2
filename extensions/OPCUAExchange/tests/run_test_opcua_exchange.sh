#!/bin/sh

# '--' - нужен для отделения аргументов catch, от наших..
cd ../../../Utilities/Admin/
./uniset2-start.sh -f ./create_links.sh
./uniset2-start.sh -f ./create

./uniset2-start.sh -f ./exist | grep -q UNISET_PLC/Controllers || exit 1
cd -

./uniset2-start.sh -f ./run_test_opcua_exchange -- --confile opcua-exchange-test-configure.xml --e-startup-pause 10 \
--opcua-name OPCUAExchange1 \
--smemory-id SharedMemory \
--opcua-s-filter-field opc \
--opcua-s-filter-value 1
#--opcua-log-add-levels level6,level7
