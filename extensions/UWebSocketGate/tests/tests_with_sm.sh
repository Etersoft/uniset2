#!/bin/sh

# '--' - нужен для отделения аргументов catch, от наших..
cd ../../../Utilities/Admin/
./uniset2-start.sh -f ./create_links.sh
./uniset2-start.sh -f ./create

./uniset2-start.sh -f ./exist | grep -q UNISET_PLC/Controllers || exit 1
cd -

./uniset2-start.sh -f ./tests-with-sm $* -- --confile uwebsocketgate-test-configure.xml --e-startup-pause 10 \
--ws-name UWebSocketGate1 --ws-httpserverhost-addr 127.0.0.1 --ws-httpserver-port 8081
#--ws-log-add-levels any


