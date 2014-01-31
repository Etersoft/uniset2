#!/bin/sh

uniset-start.sh -f ./uniset-smdbserver --confile test.xml \
	--dbserver-name DBServer \
	--dlog-add-levels info,crit,warn
	
