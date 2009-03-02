#! /bin/sh

#set -x
rm -f po/stamp-po
aclocal || exit 1
autoheader || exit 1
libtoolize --copy || exit 1
automake --add-missing --include-deps --copy || exit 1
autoconf || exit 1
test -f Makefile && ./configure --enable-maintainer-mode
exit 0
