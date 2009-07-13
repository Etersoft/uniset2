#!/bin/sh

uniset-start.sh -f ./uniset-mbslave --mbs-name MBSlave1 --confile test.xml --dlog-add-levels info,crit,warn \
	--mbs-type RTU --mbs-dev /dev/cbsideA0 --mbs-reg-from-id 1
#	--mbs-type TCP --mbs-inet-addr 127.0.0.2 --mbs-inet-port 2048