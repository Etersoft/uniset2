#!/bin/sh

./uniset-start.sh -f "./uniset-admin --confile conf21300.xml --`basename $0 .sh` $1 $2 $3 $4"

exit $?
