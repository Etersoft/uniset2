#!/bin/sh

function usage()
{	
	echo "${0##*/} SID state tv_sec tv_usec"
}


RETVAL=0
sid=$1
state=$2
tv_sec=$3
tv_usec=$4


# action
echo "event: $sid $state $tv_sec $tv_usec"



exit $RETVAL
