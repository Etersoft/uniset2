#!/bin/sh

SID=$1

[ -z "$SID" ] && SID=1

uniset-start.sh -f ./ui --confile test.xml --sid $SID
#--unideb-add-levels system,info,level9

