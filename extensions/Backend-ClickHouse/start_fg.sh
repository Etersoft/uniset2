#!/bin/sh

uniset2-start.sh -f ./uniset2-backend-clickhouse --confile test.xml \
	--clickhouse-name BackendClickHouse \
	--clickhouse-buf-maxsize 5 \
	--clickhouse-log-add-levels any $*
