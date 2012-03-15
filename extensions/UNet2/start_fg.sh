#!/bin/sh

uniset-start.sh -f ./uniset-unetexchange --unet-name UNetExchange \
	--confile test.xml --smemory-id SharedMemory \
	--unet-filter-field rs --unet-filter-value 2 --unet-maxdifferense 40 \
	--dlog-add-levels info,crit,warn

