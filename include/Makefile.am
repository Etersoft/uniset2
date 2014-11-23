############################################################################
# This file is part of the UniSet library								  #
############################################################################

SUBDIRS=modbus

# install
devel_include_HEADERS = *.h *.hh *.tcc
devel_includedir = $(pkgincludedir)

all-local:
	test -L extensions || ln -s ../extensions/include extensions
