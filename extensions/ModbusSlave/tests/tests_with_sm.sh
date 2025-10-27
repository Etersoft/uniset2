#!/bin/sh

# '--' - нужен для отделения аргументов catch, от наших..
cd ../../../Utilities/Admin/
./uniset2-start.sh -f ./create_links.sh
./uniset2-start.sh -f ./create

./uniset2-start.sh -f ./exist | grep -q UNISET_PLC/Controllers || exit 1
cd -

./uniset2-start.sh -f ./tests-with-sm $* -- --confile mbslave-test-configure.xml --e-startup-pause 10 \
--mbs-name MBSlave1 --mbs-type TCP --mbs-inet-addr 127.0.0.1 --mbs-inet-port 20048 \
--mbs-askcount-id SVU_AskCount_AS --mbs-respond-id RespondRTU_S --mbs-respond-invert 1 \
--mbs-filter-field mbs --mbs-filter-value 1 --mbs-initPause 100 \
--activator-run-httpserver --activator-httpserver-host 127.0.0.1 --activator-httpserver-port 9091 --mbs-http-enabled-setparams 1

# --mbs-log-add-levels any
                                                                            