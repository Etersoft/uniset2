#!/bin/sh

ulimit -Sc 10000000
START=uniset2-start.sh

${START} -f ./uniset2-smemory-plus --smemory-id SharedMemory  --confile test.xml \
	 --dlog-add-levels any \
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
     --add-mbmaster2 \
     --mbtcp2-name MBMaster2 \
     --mbtcp2-confnode MBMaster1 \
     --mbtcp2-filter-field rs \
     --mbtcp2-filter-value 1 \
     --mbtcp2-gateway-iaddr localhost \
     --mbtcp2-gateway-port 2049 \
     --mbtcp2-recv-timeout 200 \
     --mbtcp2-force-out 1 \
     $*
#	 --add-rtu \
#	 --rs-dev /dev/cbsideA1 \
#	 --rs-id RTUExchange \
#	 --add-mbslave \
