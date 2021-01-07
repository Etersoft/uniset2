#!/bin/sh

HTTP_RESOLVER_PID=

function atexit()
{
	trap - EXIT

	[ -n "$HTTP_RESOLVER_PID" ] && kill $HTTP_RESOLVER_PID 2>/dev/null
	sleep 3
	[ -n "$HTTP_RESOLVER_PID" ] && kill -9 $HTTP_RESOLVER_PID 2>/dev/null

	exit $RET
}

trap atexit EXIT

# '--' - нужен для отделения аргументов catch, от наших..
cd ../../../Utilities/Admin/
./uniset2-start.sh -f ./create_links.sh
./uniset2-start.sh -f ./create

./uniset2-start.sh -f ./exist | grep -q UNISET_PLC/Controllers || exit 1
cd -

../uniset2-httpresolver --confile uresolver-test-configure.xml &
HTTP_RESOLVER_PID=$!

sleep 5

./uniset2-start.sh -f ./run_test_uresolver $* -- --confile uresolver-test-configure.xml && RET=0 || RET=1

exit $RET
