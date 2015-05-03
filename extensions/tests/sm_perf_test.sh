#!/bin/sh

# '--' - нужен для отделения аргументов catch, от наших..
cd ../../Utilities/Admin/

./create_links.sh
./uniset2-start.sh -f ./create

./uniset2-start.sh -f ./exist | grep -q UNISET_PLC/Controllers || exit 1

cd -

time -p ./uniset2-start.sh -f ./sm_perf_test $* --confile tests_with_sm.xml --e-startup-pause 10
