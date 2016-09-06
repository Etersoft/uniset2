#!/bin/sh

BEG=6100

for N in `seq 1 200`; do
	ID=$((BEG+N))
	echo "			<item id=\"${ID}\" name=\"MBTCP${N}\"/>"
done