#!/bin/sh

START=uniset-start.sh
${START} -f "./uniset-admin --confile test.xml --`basename $0 .sh` $1 $2 $3 $4"
