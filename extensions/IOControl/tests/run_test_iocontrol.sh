#!/bin/sh

# '--' - нужен для отделения аргументов catch, от наших..
cd ../../../Utilities/Admin/
./uniset2-start.sh -f ./create_links.sh
./uniset2-start.sh -f ./create

./uniset2-start.sh -f ./exist | grep -q UNISET_PLC/Controllers || exit 1
cd -

./uniset2-start.sh -f ./run_test_iocontrol $* -- --confile iocontrol-test-configure.xml --e-startup-pause 10 \
--io-name IOControl1 \
--smemory-id SharedMemory \
--io-s-filter-field io \
--io-s-filter-value 1
#--io-log-add-levels any
