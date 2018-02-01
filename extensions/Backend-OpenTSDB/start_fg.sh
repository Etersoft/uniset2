#!/bin/sh

uniset2-start.sh -f ./uniset2-backend-opentsdb --confile test.xml \
	--opentsdb-name BackendOpenTSDB \
	--opentsdb-log-add-levels any $*
