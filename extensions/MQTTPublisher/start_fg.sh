#!/bin/sh

uniset2-start.sh -f ./uniset2-mqttpublisher --confile test.xml \
	--mqtt-name MQTTPublisher1 \
	--mqtt-filter-field mqtt \
	--mqtt-filter-value 1 \
	--mqtt-log-add-levels any $*
