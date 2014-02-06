#!/bin/sh

./uniset2-start.sh -f "./uniset2-admin --confile test.xml --`basename $0 .sh` $1 $2 $3 $4"

exit $?
