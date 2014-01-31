#!/bin/sh

START=uniset2-start.sh

${START} -vcall ./.libs/lt-smemory-test --confile ./test.xml --dlog-add-levels level1 --localNode LocalhostNode $*

#--ulog-add-levels crit,warn,info

#system,level2,level8,level9
