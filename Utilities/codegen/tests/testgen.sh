#!/bin/sh

cd ..

./uniset-codegen --local-include -l --ask -n TestGen tests/testgen.src.xml
mv -f TestGen* tests/

cd -