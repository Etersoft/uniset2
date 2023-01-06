############################################################################
# This file is part of the UniSet library								  #
############################################################################

SUBDIRS=modbus

# install
devel_include_HEADERS = $(wildcard $(srcdir)/*.h $(srcdir)/*.hh $(srcdir)/*.tcc)
devel_includedir = $(pkgincludedir)

all-local:
	test -L extensions || ln -s ../extensions/include extensions

update-json:
	wget -O $(top_builddir)/include/json.hpp https://github.com/nlohmann/json/raw/master/src/json.hpp

include $(top_builddir)/include.mk

