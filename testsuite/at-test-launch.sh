#!/bin/sh

[ -n "$3" ] && exit $3

cd $1
exec ./$2
exit 1
                                           