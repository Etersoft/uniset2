#!/bin/sh

uniset-start.sh -f ./uniset-mbslave --confile test.xml --dlog-add-levels info,crit,warn \
	--smemory-id SharedMemory \
	--mbs-name MBSlave1 \
	--mbs-type TCP --mbs-inet-addr 127.0.0.1 --mbs-inet-port 2048 --mbs-reg-from-id 1 --mbs-my-addr 0x01 \
	--mbs-askcount-id SVU_AskCount_AS --mbs-respond-id RespondRTU_S --mbs-respond-invert 1
	# --mbs-force 1
	#--mbs-reg-from-id 1 \
	#--mbs-filter-field CAN2sender --mbs-filter-value SYSTSNode \