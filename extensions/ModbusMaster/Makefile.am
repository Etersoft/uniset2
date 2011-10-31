noinst_LTLIBRARIES 		= libMBMaster.la
libMBMaster_la_LIBADD	= $(top_builddir)/lib/libUniSet.la \
								$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
								$(top_builddir)/extensions/lib/libUniSetExtensions.la \
								$(SIGC_LIBS)
libMBMaster_la_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)
libMBMaster_la_SOURCES 	= MBExchange.cc RTUStorage.cc

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

all-local:
	ln -sf ../ModbusMaster/$(devel_include_HEADERS) ../include
