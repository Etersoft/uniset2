#!/bin/sh

uniset2-start.sh -f ./uniset2-plogicproc --schema schema.xml \
--smemory-id SharedMemory --name LProcessor \
--confile test.xml --ulog-add-levels info,crit,warn,level9,system

