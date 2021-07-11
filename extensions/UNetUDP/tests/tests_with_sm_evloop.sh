#!/bin/sh

# '--' - нужен для отделения аргументов catch, от наших..
cd ../../../Utilities/Admin/
./uniset2-start.sh -f ./create_links.sh
./uniset2-start.sh -f ./create

./uniset2-start.sh -f ./exist | grep -q UNISET_PLC/Controllers || exit 1
cd -

./uniset2-start.sh -f ./tests-with-sm $* -- --confile unetudp-test-configure.xml --e-startup-pause 10 \
--unet-name UNetExchange --unet-filter-field unet --unet-filter-value 1 --unet-maxdifferense 5 \
--unet-recv-timeout 1000 --unet-sendpause 500 --unet-update-strategy evloop 
#--unet-log-add-levels any --ulog-add-levels any
#--unet-log-add-levels any
