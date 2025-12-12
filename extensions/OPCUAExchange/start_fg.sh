#!/bin/sh

uniset2-start.sh -f ./uniset2-opcua-exchange --confile test.xml \
	--smemory-id SharedMemory \
	--opcua-name OPCUAClient1 --opcua-confnode OPCUAClient \
	--opcua-filter-field opc \
	--opcua-log-add-levels any,warn,crit,level5,level4,level6,level3,level7,level8 \
	--opcua-polltime 300 --opcua-updatetime 300 \
	--activator-run-httpserver --activator-httpserver-host 127.0.0.1 --activator-httpserver-port 9292 --opcua-http-enabled-setparams 1
#	$*
