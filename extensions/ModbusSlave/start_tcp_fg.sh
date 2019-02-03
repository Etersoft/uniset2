#!/bin/sh

uniset2-start.sh -f ./uniset2-mbslave --confile test.xml  \
	--smemory-id SharedMemory \
	--mbs-name MBSlave1 \
	--mbs-type TCP --mbs-inet-addr 127.0.0.1 --mbs-inet-port 2048 --mbs-reg-from-id 1 --mbs-my-addr 0x01 \
	--mbs-askcount-id SVU_AskCount_AS --mbs-respond-id RespondRTU_S --mbs-respond-invert 1 --ulog-add-levels system \
	--mbs-log-add-levels any $*
	# --mbs-exchangelog-add-levels any $*
	# --mbs-force 1
	#--mbs-reg-from-id 1 \
	#--mbs-filter-field CAN2sender --mbs-filter-value SYSTSNode \