#!/bin/sh

uniset2-start.sh -f ./uniset2-unetexchange --unet-name UNetExchange \
	--confile test.xml --smemory-id SharedMemory \
	--unet-filter-field rs --unet-filter-value 2 --unet-maxdifferense 40 \
	--dlog-add-levels info,crit,warn 
	
#--unet-nodes-confnode specnet
