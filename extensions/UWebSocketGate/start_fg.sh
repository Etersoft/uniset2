#!/bin/sh

ulimit -Sc 1000000

<<<<<<< HEAD
uniset2-start.sh -f ./uniset2-wsgate --confile test.xml --ws-name UWebSocketGate1 --ws-log-add-levels any $*
=======
uniset2-start.sh -f ./uniset2-wsgate --confile test.xml --ws-name UWebSocketGate1 --ws-log-add-levels any --ws-log-verbosity 5 $*
>>>>>>> 2.10.1-alt1
