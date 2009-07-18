#!/bin/sh

uniset-start.sh -f ./uniset-mbtcpmaster --mbtcp-name MBMaster1 --confile test.xml \
--dlog-add-levels info,crit,warn \
--mbtcp-iaddr 127.0.0.1 --mbtcp-port 2048
#--mbtcp-filter-field mbtcp --mbtcp-filter-value 1

#--mbtcp-reg-from-id 1