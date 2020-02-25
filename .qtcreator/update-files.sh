#!/bin/sh

find ../ -type f -name '*.cc' -or -name '*.cpp' -or -name '*.h' -or -name '*.src.xml' -or -name '*.am' | grep -v '_SK' | sort -n > uniset2.files
echo ../conf/test.xml >> uniset2.files
