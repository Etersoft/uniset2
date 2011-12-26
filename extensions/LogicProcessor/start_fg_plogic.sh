#!/bin/sh

uniset-start.sh -f ./uniset-plogicproc --schema schema.xml \
--smemory-id SharedMemory --name LProcessor \
--confile test.xml --unideb-add-levels info,crit,warn,level9,system

