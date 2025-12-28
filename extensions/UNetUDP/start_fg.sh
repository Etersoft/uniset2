#!/bin/sh

uniset2-start.sh -f ./uniset2-unetexchange --unet-name UNetExchange --unet-run-logserver \
	--confile test.xml --smemory-id SharedMemory \
	--unet-filter-field rs --unet-filter-value 2 --unet-maxdifferense 40 --unet-sendpause 1000 \
	--dlog-add-levels info,crit,warn --unet-log-add-levels info,crit,warn,any --unet-S-localhost-3000-add-levels any \
	--activator-run-httpserver --activator-httpserver-host 127.0.0.1 --activator-httpserver-port 9696 $*
#--unet-nodes-confnode specnet
