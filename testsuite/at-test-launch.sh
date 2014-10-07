#!/bin/sh

[ -n "$3" ] && exit $3

echo "$1 run $2" > e.t

cd $1
exec ./$2
exit 1
                                           