#!/bin/sh

uniset2-start.sh -f ./uniset2-opcuagate --confile test.xml \
	--opcua-name OPCUAGate1 --opcua-confnode OPCUAGate \
	--opcua-log-add-levels any \
	--opcua-s-filter-field opcua --opcua-s-filter-value 1 \
	$*
