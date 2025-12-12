#!/bin/sh

uniset2-start.sh -f ./uniset2-mbslave --mbs-name MBSlave1 --confile test.xml --dlog-add-levels info,crit,warn \
	--mbs-type RTU --mbs-dev /dev/cbsideA0 --mbs-reg-from-id 1 \
	--activator-run-httpserver --activator-httpserver-port 9090
#	--mbs-type TCP --mbs-inet-addr 127.0.0.2 --mbs-inet-port 2048

# --activator-run-httpserver --activator-httpserver-host 127.0.0.1 --activator-httpserver-port 9090