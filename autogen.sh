#! /bin/sh

autoreconf -fiv

# set flags as in rpm build
CFLAGS='-pipe -O2 -march=i586 -mtune=i686'
export CFLAGS
CXXFLAGS="$CFLAGS"
export CXXFLAGS

./configure --enable-maintainer-mode --prefix=/usr $*
