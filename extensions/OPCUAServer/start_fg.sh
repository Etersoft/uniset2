#!/bin/sh

uniset2-start.sh -f ./uniset2-opcua-server --confile test.xml \
	--opcua-name OPCUAServer1 --opcua-confnode OPCUAServer \
	--opcua-filter-field opcua --opcua-filter-value 1 \
	--opcua-log-add-levels any,-level6,-level8 \
	--activator-run-httpserver --activator-httpserver-host 127.0.0.1 --activator-httpserver-port 9090 --mbtcp-http-enabled-setparams \
	$*
