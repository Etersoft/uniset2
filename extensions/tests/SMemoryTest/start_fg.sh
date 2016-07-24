#!/bin/sh

START=uniset2-start.sh

${START} -f ./smemory-test --confile ./test.xml --dlog-add-levels level1 --localNode LocalhostNode \
--sm-log-add-levels any $* --sm-run-logserver --TestProc1-run-logserver --uniset-abort-script ./abort-script-example.sh
#--ulog-add-levels crit,warn,info

#system,level2,level8,level9
