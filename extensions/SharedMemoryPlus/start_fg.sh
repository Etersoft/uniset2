#!/bin/sh

ulimit -Sc 10000000
START=uniset2-start.sh

MBMS=""
for n in `seq 1 10`; do
	p=`echo "2050+$n" | bc`
    MBMS="$MBMS --add-mbmultislave$n --mbms$n-name MBMultiSlave$n --mbms$n-confnode MBMultiSlave1 --mbms$n-type TCP --mbms$n-inet-addr 127.0.0.1 --mbms$n-inet-port $p --mbms$n-reg-from-id 1 --mbms$n-my-addr 0x01"
done

MBS=""
for n in `seq 1 5`; do
	p=`echo "2090+$n" | bc`
    MBS="$MBS --add-mbslave$n --mbs$n-name MBSlave$n --mbs$n-confnode MBMultiSlave1 --mbs$n-type TCP --mbs$n-inet-addr 127.0.0.1 --mbs$n-inet-port $p --mbs$n-reg-from-id 1 --mbs$n-my-addr 0x01"
done

${START} -f ./uniset2-smemory-plus --smemory-id SharedMemory  --confile test.xml \
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
     $MBMS \
	 $MBS \
     --ulog-add-levels system \
     --add-unet \
	--unet-name UNetExchange --unet-run-logserver \
    --unet-filter-field rs --unet-filter-value 2 --unet-maxdifferense 40 --unet-sendpause 1000     
     $*
#	 --add-rtu \
#	 --rs-dev /dev/cbsideA1 \
#	 --rs-id RTUExchange \
#	 --add-mbslave \
