#!/bin/sh

uniset-start.sh -f ./uniset-mbslave --mbs-name MBSlave1 --confile test.xml --dlog-add-levels info,crit,warn \
	--mbs-name MBSlave1 \
	--mbs-type TCP --mbs-inet-addr 127.0.0.1 --mbs-inet-port 2048 --mbs-reg-from-id 1 --mbs-my-addr 0x01 \
	--mbs-askcount-id SVU_AskCount_AS
	# --mbs-force 1
	#--mbs-reg-from-id 1 \