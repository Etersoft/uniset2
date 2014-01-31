#!/bin/sh

uniset-start.sh -f ./uniset-mbslave --confile test.xml --dlog-add-levels info,crit,warn \
	--mbs-name MBSlave1 \
	--mbs-type TCP --mbs-inet-addr 127.0.0.1 --mbs-inet-port 2048 --mbs-my-addr 0x01 \
	--mbs-askcount-id SVU_AskCount_AS \
	--mbs-filter-field mbtcp --mbs-filter-value 2