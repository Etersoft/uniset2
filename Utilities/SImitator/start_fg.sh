#!/bin/sh

ulimit -Sc 1000000

uniset-start.sh -f ./uniset-simitator --confile test.xml --sid 10,16
#--unideb-add-levels info,crit,warn,level9,system

