#!/bin/sh

ulimit -Sc 1000000

uniset-start.sh -f ./uniset-nullController --name SharedMemory --confile test.xml --unideb-add-levels info,crit,warn,level9,system
#info,warn,crit,system,level9 > 1.log
#--c-filter-field cfilter --c-filter-value test1 --s-filter-field io --s-filter-value 1

