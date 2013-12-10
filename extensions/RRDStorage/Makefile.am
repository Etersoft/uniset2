bin_PROGRAMS 			= @PACKAGE@-rrdstorage

RRD_VER=@LIBVER@

lib_LTLIBRARIES 			= libUniSetRRDStorage.la
libUniSetRRDStorage_la_LDFLAGS  = -version-info $(RRD_VER)
libUniSetRRDStorage_la_LIBADD		= $(top_builddir)/lib/libUniSet.la \
									$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSetExtensions.la \
									$(SIGC_LIBS) $(RRD_LIBS)
libUniSetRRDStorage_la_CXXFLAGS	= -I$(top_builddir)/extensions/include \
									-I$(top_builddir)/extensions/SharedMemory \
									$(SIGC_CFLAGS) $(RRD_CFLAGS)
libUniSetRRDStorage_la_SOURCES 	= RRDStorage.cc

@PACKAGE@_rrdstorage_SOURCES 	= main.cc
@PACKAGE@_rrdstorage_LDADD 	= libUniSetRRDStorage.la $(top_builddir)/lib/libUniSet.la \
									$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSetExtensions.la \
									$(SIGC_LIBS) $(RRD_LIBS)
@PACKAGE@_rrdstorage_CXXFLAGS	= -I$(top_builddir)/extensions/include \
									-I$(top_builddir)/extensions/SharedMemory \
									$(SIGC_CFLAGS) $(RRD_CFLAGS)

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSetRRDStorage.pc

all-local:
	ln -sf ../RRDStorage/$(devel_include_HEADERS) ../include
