#!/bin/sh

ulimit -Sc 1000000000000

uniset-start.sh -f ./uniset-smemory --smemory-id SharedMemory1 \
--confile test.xml \
--unideb-add-levels info,crit,warn,level9,system

