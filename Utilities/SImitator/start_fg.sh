#!/bin/sh

ulimit -Sc 1000000

#for i in `seq 1 20`; 
#do
	uniset2-start.sh -f ./uniset2-simitator --confile test.xml --sid $*
#done

#wait

#--ulog-add-levels info,crit,warn,level9,system

