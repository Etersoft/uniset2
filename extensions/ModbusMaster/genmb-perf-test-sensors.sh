#!/bin/sh

BEG=10000
STEP=50
MBCNT=100

for M in `seq 1 $MBCNT`; do
	R=1
	for I in `seq 1 $STEP`; do
		ID=$((BEG+I))
		
		# делаем "дырки" между регистрами, чтобы они не шли в одном запросе
		R=$((R+2)) 
		echo "			<item id=\"$ID\" mbperf=\"$M\" iotype=\"AI\" mbaddr=\"0x01\" mbfunc=\"0x03\" mbreg=\"$R\" mbtype=\"rtu\" name=\"Sensor${ID}_S\" textname=\"test sensor $ID\"/>"
	done
	BEG=$((BEG+STEP+1))
done