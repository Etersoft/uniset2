#!/bin/sh

export LD_LIBRARY_PATH="../../lib/.libs;../lib/.libs"

ulimit -Sc 10000000000

./uniset2-start.sh -f ./uniset2-network --confile test.xml \
 --smemory-id SharedMemory \
 --unet-id UniExchange

