#! /bin/sh

autoreconf -fiv

export CXXFLAGS='-pipe -O2 -pedantic -Wall -std=c++11'

./configure --enable-maintainer-mode --prefix=/usr $*
