#!/bin/sh

# '--' - нужен для отделения аргументов catch, от наших..
cd ../../../Utilities/Admin/
./uniset2-start.sh -f ./create_links.sh
./uniset2-start.sh -f ./create

./uniset2-start.sh -f ./exist | grep -q UNISET_PLC/Controllers || exit 1
cd -

./uniset2-start.sh -f ./tests-with-sm -- --apart --confile opcua-server-test-configure.xml \
--opcua-filter-field iotype --smemory-id SharedMemory \
--activator-run-httpserver --activator-httpserver-host 127.0.0.1 --activator-httpserver-port 9090 --opcua-http-enabled-setparams 1 $*

# --opcua-log-add-levels any
