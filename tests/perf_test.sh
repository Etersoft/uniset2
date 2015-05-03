#!/bin/sh

# '--' - нужен для отделения аоргументов catch, от наших..
./perf_test $* -- --confile tests_with_conf.xml --prop-id2 -10 --ulog-no-debug && exit 0 || exit 1
