#!/bin/sh

# '--' - нужен для отделения аргументов catch, от наших..
cd ../../../Utilities/Admin/
./uniset2-start.sh -f ./create_links.sh
./uniset2-start.sh -f ./create

./uniset2-start.sh -f ./exist | grep -q UNISET_PLC/Controllers || exit 1
cd -

./uniset2-start.sh -f ./tests-with-sm $* -- --confile jscript-test-configure.xml --js-file main-test.js --js-name JSProxy1 --js-confnode JSProxy --js-loopCount 100 

# --js-log-add-levels any 
# --sm-log-add-levels any 
# --ulog-add-levels any
