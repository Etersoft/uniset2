#!/bin/sh

# '--' - нужен для отделения аргументов catch, от наших..
cd ../../Utilities/Admin/

./create_links.sh
./uniset2-start.sh -f ./create

./uniset2-start.sh -f ./exist | grep -q UNISET_PLC/Controllers || exit 1

cd -

./uniset2-start.sh -f ./tests_with_sm $* -- --confile tests_with_sm.xml --e-startup-pause 10 --http-api-disable-access-control 1 --activator-run-httpserver --activator-httpserver-port 9191
# --ulog-add-levels any
