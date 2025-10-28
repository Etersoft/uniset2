#!/bin/sh

# '--' - нужен для отделения аргументов catch, от наших..
cd ../../../Utilities/Admin/
./uniset2-start.sh -f ./create_links.sh
./uniset2-start.sh -f ./create

./uniset2-start.sh -f ./exist | grep -q UNISET_PLC/Controllers || exit 1
cd -

./uniset2-start.sh -f ./test-method-with-sm $* -- --confile opcua-server-test-method-configure.xml --e-startup-pause 10 \
--activator-run-httpserver --activator-httpserver-host 127.0.0.1 --activator-httpserver-port 9090 --opcua-http-enabled-setparams 1 \
--opcua-filter-field iotype --smemory-id SharedMemory $*

# --opcua-log-add-levels any
