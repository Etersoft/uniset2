#!/bin/sh

# '--' - нужен для отделения аоргументов catch, от наших..
./tests_with_conf $* -- --confile tests_with_conf.xml --prop-id2 -10 --ulog-no-debug && exit 0 || exit 1
