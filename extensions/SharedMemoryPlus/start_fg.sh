#!/bin/sh

ulimit -Sc 10000000
START=uniset-start.sh

${START} -f ./uniset-smemory-plus --smemory-id SharedMemory  --confile test.xml \
	 --unideb-add-levels info,crit,warn,level9,system,level3,level2,level1 \
	 --io-name IOControl \
	 --io-polltime 100 \
	 --io-s-filter-field io \
	 --io-s-filter-value 1 \
	 --iodev1 /dev/null \
	 --iodev2 /dev/null \
	 --io-test-lamp RS_Test9_S \
	 --io-heartbeat-id AI_AS \
	 --io-sm-ready-test-sid RS_Test9_S \
     --add-mbmaster \
     --mbtcp-name MBMaster1 \
     --mbtcp-filter-field rs \
     --mbtcp-filter-value 1 \
     --mbtcp-gateway-iaddr localhost \
     --mbtcp-gateway-port 2048 \
     --mbtcp-recv-timeout 200 \
     --mbtcp-force-out 1 \
#	 --add-rtu \
#	 --rs-dev /dev/cbsideA1 \
#	 --rs-id RTUExchange \
#	 --add-mbslave \
	 

#--skip-rtu1 --skip-rtu2 --skip-can --dlog-add-levels info,warn,crit 
#--c-filter-value 1 --unideb-add-levels level9,warn,crit,info --dlog-add-levels info,warn,crit
#--unideb-add-levels info,warn,crit,system,repository

