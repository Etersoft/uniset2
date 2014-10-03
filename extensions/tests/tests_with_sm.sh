#!/bin/sh

# '--' - нужен для отделения аоргументов catch, от наших..
uniset-start.sh -f ./tests_with_sm $* -- --confile tests_with_sm.xml --e-startup-pause 10 --ulog-levels warn,crit --dlog-levels warn,crit 
