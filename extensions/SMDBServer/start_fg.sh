#!/bin/sh

uniset2-start.sh -f ./uniset2-smdbserver --confile test.xml \
	--dbserver-name DBServer \
	--dlog-add-levels info,crit,warn
	
