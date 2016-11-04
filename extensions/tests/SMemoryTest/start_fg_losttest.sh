#!/bin/sh

START=uniset2-start.sh

LOGLEVEL="crit,warn,info"

${START} -f ./sm-lostmessage-test --confile ./test-lost.xml \
--numproc 3 \
--TestProc1-filter-field losttest \
--TestProc1-filter-value 1 \
--TestProc1-sleep-msec 50 \
--TestProc1-log-add-levels $LOGLEVEL \
--TestProc2-filter-field losttest \
--TestProc2-filter-value 2 \
--TestProc2-sleep-msec 50 \
--TestProc2-log-add-levels $LOGLEVEL \
--TestProc3-filter-field losttest \
--TestProc3-filter-value 3 \
--TestProc3-sleep-msec 50 \
--TestProc3-log-add-levels $LOGLEVEL \
--ulog-add-levels crit $*
# --uniset-abort-script ./abort-script-example.sh
#--ulog-add-levels crit,warn,info

#system,level2,level8,level9
