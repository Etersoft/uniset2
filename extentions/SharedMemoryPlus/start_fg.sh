#!/bin/sh

ulimit -Sc 10000000
START=uniset-start.sh

${START} -f ./uniset-smemory-plus --smemory-id SharedMemory  --confile test.xml \
	 --unideb-add-levels info,crit,warn,level9,system \
	 --add-io \
	 --io-force 1 \
	 --io-name IOControl1 \
	 --io-polltime 100 \
	 --io-s-filter-field io \
	 --io-s-filter-value ts \
	 --iodev1 /dev/null 
	 --iodev2 /dev/null \
	 --io-test-lamp TS_TestLamp_S \
	 --io-heartbeat-id _31_11_AS \
	 --io-sm-ready-test-sid TestMode_S \
	 --add-rtu \
	 --rtu-device /dev/cbsideA1 \
	 --rtu-id RTUExchange \
	 --add-mbslave \
	 

#--skip-rtu1 --skip-rtu2 --skip-can --dlog-add-levels info,warn,crit 
#--c-filter-value 1 --unideb-add-levels level9,warn,crit,info --dlog-add-levels info,warn,crit
#--unideb-add-levels info,warn,crit,system,repository

