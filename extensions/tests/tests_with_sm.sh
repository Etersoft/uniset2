#!/bin/sh

# '--' - нужен для отделения аоргументов catch, от наших..
uniset-start.sh -f ./tests_with_sm $* -- --confile tests_with_sm.xml
