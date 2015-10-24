#! /bin/sh

autoreconf -fiv

export CXXFLAGS='-pipe -O2 -pedantic -Wall'

./configure --enable-maintainer-mode --prefix=/usr $*
