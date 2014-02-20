#!/bin/sh

uniset2-start.sh -f ./uniset2-mbtcp-multislave --confile test.xml --dlog-add-levels level3,level4 \
	--smemory-id SharedMemory \
	--mbs-name MBMultiSlave1 --mbs-type TCP --mbs-inet-addr 127.0.0.1 --mbs-inet-port 2048 --mbs-reg-from-id 1 --mbs-my-addr 0x01 \
	$* 
	# --mbs-force 1
	#--mbs-reg-from-id 1 \
	#--mbs-filter-field CAN2sender --mbs-filter-value SYSTSNode \