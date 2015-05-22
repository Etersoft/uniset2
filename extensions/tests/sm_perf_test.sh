#!/bin/sh

# '--' - нужен для отделения аргументов catch, от наших..
cd ../../Utilities/Admin/

./create_links.sh
./uniset2-start.sh -f ./create

./uniset2-start.sh -f ./exist | grep -q UNISET_PLC/Controllers || exit 1

cd -

#time -p ./uniset2-start.sh -vcall --dump-instr=yes --simulate-cache=yes --collect-jumps=yes ./sm_perf_test $* --confile sm_perf_test.xml --e-startup-pause 10
./uniset2-start.sh -f ./sm_perf_test $* --confile sm_perf_test.xml --e-startup-pause 10 > sm_test_gprof.log
