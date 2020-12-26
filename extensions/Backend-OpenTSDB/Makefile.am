if ENABLE_OPENTSDB
bin_PROGRAMS = @PACKAGE@-backend-opentsdb

TSDB_VER=@LIBVER@

lib_LTLIBRARIES = libUniSet2BackendOpenTSDB.la
libUniSet2BackendOpenTSDB_la_LDFLAGS  = -version-info $(TSDB_VER)
libUniSet2BackendOpenTSDB_la_LIBADD		= $(top_builddir)/lib/libUniSet2.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
									$(SIGC_LIBS)
libUniSet2BackendOpenTSDB_la_CXXFLAGS	= -I$(top_builddir)/extensions/include \
									-I$(top_builddir)/extensions/SharedMemory \
									$(SIGC_CFLAGS)
libUniSet2BackendOpenTSDB_la_SOURCES 	= BackendOpenTSDB.cc

@PACKAGE@_backend_opentsdb_SOURCES 	= main.cc
@PACKAGE@_backend_opentsdb_LDADD 	= libUniSet2BackendOpenTSDB.la $(top_builddir)/lib/libUniSet2.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
									$(SIGC_LIBS)
@PACKAGE@_backend_opentsdb_CXXFLAGS	= -I$(top_builddir)/extensions/include \
									-I$(top_builddir)/extensions/SharedMemory \
									$(SIGC_CFLAGS)

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet2BackendOpenTSDB.pc

include $(top_builddir)/include.mk

endif
