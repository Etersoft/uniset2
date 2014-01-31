############################################################################
# This file is part of the UniSet library                                  #
############################################################################

SUBDIRS=IOs modbus
include $(top_builddir)/conf/common.mk


# install
devel_include_HEADERS = *.h *.hh
devel_includedir = $(pkgincludedir)

all-local:
	test -L extensions || ln -s ../extensions/include extensions
