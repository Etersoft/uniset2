#!/bin/sh


for p in `seq 50001 50010`; do
	uniset2-unet-udp-tester -s 127.255.255.255:$p -x 50 &
done
