#!/bin/sh

# '--' - нужен для отделения аргументов catch, от наших..
cd ../../../Utilities/Admin/
./uniset2-start.sh -f ./create_links.sh
./uniset2-start.sh -f ./create

./uniset2-start.sh -f ./exist | grep -q UNISET_PLC/Controllers || exit 1
cd -

./uniset2-start.sh -f ./run_test_opcua_thr $* -- --confile opcua-thr-test-configure.xml --e-startup-pause 10 \
--activator-run-httpserver --activator-httpserver-host 127.0.0.1 --activator-httpserver-port 9090 --opcua-http-enabled-setparams 1 \
--opcua-name OPCUAExchange1 \
--smemory-id SharedMemory \
--opcua-filter-field opc \
--opcua-filter-value 1 \
--opcua-enable-subscription 1 \
--subscr
#--opcua-log-add-levels level5,-level6,-level7
