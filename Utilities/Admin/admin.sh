#!/bin/sh

./uniset-start.sh -f "./uniset-admin --confile test.xml --`basename $0 .sh` $1 $2 $3 $4"
