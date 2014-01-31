#!/bin/sh

uniset2-start.sh -f ./uniset2-mbslave --confile test.xml --dlog-add-levels info,crit,warn \
	--smemory-id SharedMemory \
	--mbs-name MBSlave2 --mbs-type TCP --mbs-inet-addr localhost --mbs-inet-port 2050 --mbs-reg-from-id 1 --mbs-my-addr 0x01 \
	
#	--mbs-askcount-id SVU_AskCount_AS --mbs-respond-id RespondRTU_S --mbs-respond-invert 1
