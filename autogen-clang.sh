#! /bin/sh

autoreconf -fiv

export CXX=clang++
export CC=clang

export CXXFLAGS='-pipe -O2 -pedantic -Wall'

./configure --enable-maintainer-mode --prefix=/usr $*
