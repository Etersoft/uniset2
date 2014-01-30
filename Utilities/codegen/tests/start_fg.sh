#!/bin/sh

ulimit -Sc 1000000

uniset-start.sh -f ./test --name TestProc --confile test.xml --ulog-add-levels info,crit,warn,level1,level8 \
--sm-ready-timeout 5000
#info,warn,crit,system,level9 > 1.log
#--c-filter-field cfilter --c-filter-value test1 --s-filter-field io --s-filter-value 1

