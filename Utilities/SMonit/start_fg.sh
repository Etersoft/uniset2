#!/bin/sh

uniset-start.sh -f ./uniset-smonit --name TestProc --confile test.xml --sid $*
#--ulog-add-levels system,info,level9 $*

