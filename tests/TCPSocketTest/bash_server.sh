#!/bin/sh

function usage()
{
	echo "Usage: bash_server.sh IP PORT"
	exit 0
}

[ -z "$1" ] && usage
[ -z "$2" ] && usage

nc $1 $2 -lkvD