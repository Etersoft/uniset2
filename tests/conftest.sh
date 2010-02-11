#!/bin/sh

SID=$1

[ -z "$SID" ] && SID=1

uniset-start.sh -f ./conftest --confile test.xml
#--unideb-add-levels system,info,level9

