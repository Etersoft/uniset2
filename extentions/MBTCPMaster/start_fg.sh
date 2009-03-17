#!/bin/sh

uniset-start.sh -f ./uniset-mbtcpmaster --mbtcp-name MBMaster1 --confile test.xml \
--dlog-add-levels info,crit,warn --mbtcp-reg-from-id 1 \
--mbtcp-iaddr 127.0.0.1 --mbtcp-port 30000
