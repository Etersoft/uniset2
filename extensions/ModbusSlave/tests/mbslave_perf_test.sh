#!/bin/sh

# '--' - нужен для отделения аргументов catch, от наших..
cd ../../../Utilities/Admin/
./uniset2-start.sh -f ./create_links.sh
./uniset2-start.sh -f ./create

./uniset2-start.sh -f ./exist | grep -q UNISET_PLC/Controllers || exit 1
cd -


MAX=200

PARAMS=
for n in `seq 1 $MAX`; do
   PARAMS="${PARAMS} --mbs${n}-name MBTCP${n} --mbs${n}-inet-port ${n+80000} --mbs${n}-inet-addr 127.0.0.1 \
   --mbs${n}-type TCP --mbs${n}-filter-field mbtcp --mbs${n}-filter-value 2 --mbs${n}-confnode MBSlave2 \
   --mbs${n}-log-add-levels crit,warn"
done

./uniset2-start.sh -f ./mbslave-perf-test --confile test.xml ${PARAMS} --numproc $MAX $*
                                                                            