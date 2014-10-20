#!/bin/sh

# '--' - нужен для отделения аоргументов catch, от наших..
cd ../../Utilities/Admin/
uniset-start.sh -f ./create_links.sh
uniset-start.sh -f ./create

uniset-start.sh -f ./exist | grep -q UNISET_PLC/Controllers || exit 1
cd -

uniset-start.sh -f ./tests_with_sm $* -- --confile tests_with_sm.xml --e-startup-pause 10 --ulog-levels warn,crit --dlog-levels warn,crit 
