#!/bin/sh

uniset2-start.sh -f ./uniset2-opcua-exchange --confile test.xml \
	--smemory-id SharedMemory \
	--opcua-name OPCUAClient1 --opcua-confnode OPCUAClient \
	--opcua-s-filter-field opc \
	--opcua-log-add-levels warn,crit,level5,level4,-level6,-level3,level7,level8 \
	--opcua-polltime 50 --opcua-updatetime 50 \
#	$*
