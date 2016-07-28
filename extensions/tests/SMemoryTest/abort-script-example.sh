#!/bin/sh

exec gdb --batch -n -ex "thread apply all bt" $1 $2
# | ssh xxxxxx

# gcore $2
