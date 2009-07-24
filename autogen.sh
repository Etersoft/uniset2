#! /bin/sh

autoreconf -fiv

# run configure if project is compiled
test -f Makefile && ./configure --enable-maintainer-mode --prefix=/usr
exit 0