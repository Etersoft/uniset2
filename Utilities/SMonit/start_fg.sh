#!/bin/sh

uniset2-start.sh -f ./uniset2-smonit --name TestProc --confile test.xml --sid $*
#--ulog-add-levels system,info,level9 $*

