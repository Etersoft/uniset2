#!/bin/sh

uniset-start.sh -f ./uniset-mbslave --mbs-name MBSlave1 --confile test.xml --dlog-add-levels info,crit,warn \
	--mbs-type TCP --mbs-inet-addr 127.0.0.1 --mbs-inet-port 2048 \
	--mbs-filter-field rs --mbs-filter-value 1
	# --mbs-force 1
	#--mbs-reg-from-id 1 \