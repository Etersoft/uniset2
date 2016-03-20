#!/bin/sh

START=uniset2-start.sh

${START} -f ./mbslave-test --confile ./test.xml --dlog-add-levels level1 --localNode LocalhostNode \
--sm-log-add-levels any $* --sm-run-logserver --TestProc1-run-logserver
