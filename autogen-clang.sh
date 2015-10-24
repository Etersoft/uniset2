#! /bin/sh

autoreconf -fiv

epm assure clang
export CXX=clang++
export CC=clang

export CXXFLAGS='-pipe -O2 -pedantic -Wall'

./configure --enable-maintainer-mode --prefix=/usr $*
