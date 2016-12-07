#!/bin/sh

ulimit -Sc 1000000

uniset2-start.sh -f ./test --name TestProc --confile test.xml --ulog-add-levels system,warn,crit \
--test-sm-ready-timeout 15000 --test-run-logserver $* 
#--test-log-add-levels any $*

#info,warn,crit,system,level9 > 1.log
#--c-filter-field cfilter --c-filter-value test1 --s-filter-field io --s-filter-value 1

