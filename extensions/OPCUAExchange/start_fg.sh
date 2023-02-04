#!/bin/sh

uniset2-start.sh -f ./uniset2-opcua-exchange --confile test.xml \
	--smemory-id SharedMemory \
	--opcua-name OPCUAClient1 --opcua-confnode OPCUAClient \
	--opcua-s-filter-field opc --opcua-polltime 1000 \
	--opcua-log-add-levels warn,crit,-level4,-level6,-level3,level7 \
	--opcua-polltime 500 \
#	$*
