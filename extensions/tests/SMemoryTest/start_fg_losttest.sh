#!/bin/sh

START=uniset2-start.sh

${START} -f ./sm-lostmessage-test --confile ./test.xml --TestProc1-log-add-levels crit,warn --ulog-add-levels crit $*
# --uniset-abort-script ./abort-script-example.sh
#--ulog-add-levels crit,warn,info

#system,level2,level8,level9
