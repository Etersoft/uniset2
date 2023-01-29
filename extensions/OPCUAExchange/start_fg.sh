#!/bin/sh

uniset2-start.sh -f ./uniset2-opcua-exchange --confile test.xml \
	--smemory-id SharedMemory \
	--opcua-name OPCUAClient1 --opcua-confnode OPCUAClient \
	--opcua-s-filter-field opc \
	--opcua-log-add-levels any,-level6 \
	$*
