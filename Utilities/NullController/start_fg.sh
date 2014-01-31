#!/bin/sh

ulimit -Sc 1000000

uniset2-start.sh -f ./uniset2-nullController --name SharedMemory1 --confile test.xml --ulog-add-levels any
#info,warn,crit,system,level9 > 1.log
#--c-filter-field cfilter --c-filter-value test1 --s-filter-field io --s-filter-value 1

