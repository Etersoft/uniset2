#!/bin/sh

trap '' HUP INT QUIT PIPE TERM

RET=0
LOGSERVER_PID=
LOGDB_PID=
dbfile="logdb-tests.db"
http_host="localhost"
http_port=8888

function atexit()
{
	trap - EXIT

	[ -n "$LOGSERVER_PID" ] && kill -9 $LOGSERVER_PID 2>/dev/null
	[ -n "$LOGDB_PID" ] && kill -9 $LOGDB_PID 2>/dev/null

    sleep 3

	rm -f "$dbfile*"

	exit $RET
}

trap atexit EXIT

function create_test_db()
{
	./uniset2-logdb-adm create -f $dbfile
}

function logdb_run_logserver()
{
	./uniset2-test-logserver -i localhost -p 3333 -d 500 1>/dev/null 2>/dev/null &
	LOGSERVER_PID=$!
	return $?
}

function logdb_run()
{
	./uniset2-logdb --logdb-single-confile logdb-tests-conf.xml \
	--logdb-dbfile $dbfile \
	--logdb-db-buffer-size 5 \
	--logdb-httpserver-host $http_host \
	--logdb-httpserver-port $http_port \
	--logdb-ls-check-connection-sec 1 \
	--logdb-db-max-records 20000 &

	LOGDB_PID=$!
	return $?
}

function logdb_error()
{
	printf "%20s: ERROR: %s\n" "$1" "$2"
}
# ------------------------------------------------------------------------------------------
function logdb_test_count()
{
	CNT=$( echo 'SELECT count(*) from logs;' | sqlite3 $dbfile )

	[ "$CNT" != "0" ] && return 0

	logdb_error "test_count" "count of logs should be > 0"
	return 1
}

function logdb_test_http_count()
{
	REQ=$( curl -s --request GET "http://$http_host:$http_port/api/v01/logdb/count" )
	echo $REQ | grep -q '"count":' && return 0

	logdb_error "test_http_count" "get count of records fail"
	return 1
}

function logdb_test_http_list()
{
	REQ=$( curl -s --request GET "http://$http_host:$http_port/api/v01/logdb/list" )
	echo $REQ | grep -q 'logserver1' && return 0

	logdb_error "test_http_list" "get list must contain 'logserver1'"
	return 1
}
# ------------------------------------------------------------------------------------------
function logdb_run_all_tests()
{
   logdb_run_logserver || return 1
   sleep 3
   logdb_run || return 1
   sleep 5

   # =========== ТЕСТЫ ============
   logdb_test_count || RET=1
   logdb_test_http_count || RET=1
   logdb_test_http_list || RET=1
}

create_test_db || exit 1

logdb_run_all_tests || exit 1

exit $RET
