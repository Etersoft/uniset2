#!/bin/sh

# '--' - нужен для отделения аоргументов catch, от наших..
./uniset2-start.sh -f ./tests_with_conf $* -- --confile tests_with_conf.xml --prop-id2 -10
