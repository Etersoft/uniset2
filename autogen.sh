#! /bin/sh

# If needed, run autoreconf -fiv manually and commit all files

# We run just autoreconf, updates all needed
autoreconf -v

# run configure if project is compiled
test -f Makefile && ./configure --enable-maintainer-mode
exit 0
