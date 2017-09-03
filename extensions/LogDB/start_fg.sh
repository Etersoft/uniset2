#!/bin/sh

ulimit -Sc 1000000

#uniset2-start.sh -f
./uniset2-logdb --confile test.xml --logdb-name LogDB --logdb-log-add-levels any
