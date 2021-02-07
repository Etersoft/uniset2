if ENABLE_CLICKHOUSE
bin_PROGRAMS = @PACKAGE@-backend-clickhouse

CLICKHOUSE_VER=@LIBVER@

lib_LTLIBRARIES = libUniSet2BackendClickHouse.la
libUniSet2BackendClickHouse_la_LDFLAGS  = -version-info $(CLICKHOUSE_VER)
libUniSet2BackendClickHouse_la_LIBADD		= $(top_builddir)/lib/libUniSet2.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
									$(SIGC_LIBS) $(CLICKHOUSE_LIBS)
libUniSet2BackendClickHouse_la_CXXFLAGS	= -I$(top_builddir)/extensions/include \
									-I$(top_builddir)/extensions/SharedMemory \
									$(SIGC_CFLAGS) $(CLICKHOUSE_CFLAGS)
libUniSet2BackendClickHouse_la_SOURCES 	= BackendClickHouse.cc ClickHouseInterface.cc ClickHouseTagsConfig.cc

@PACKAGE@_backend_clickhouse_SOURCES 	= main.cc
@PACKAGE@_backend_clickhouse_LDADD 	= libUniSet2BackendClickHouse.la $(top_builddir)/lib/libUniSet2.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
									$(SIGC_LIBS) $(CLICKHOUSE_LIBS)
@PACKAGE@_backend_clickhouse_CXXFLAGS	= -I$(top_builddir)/extensions/include \
									-I$(top_builddir)/extensions/SharedMemory \
									$(SIGC_CFLAGS) $(CLICKHOUSE_CFLAGS)
noinst_PROGRAMS = clickhouse-test
clickhouse_test_LDADD   = libUniSet2BackendClickHouse.la $(top_builddir)/lib/libUniSet2.la $(CLICKHOUSE_LIBS)
clickhouse_test_CXXFLAGS = $(CLICKHOUSE_CFLAGS)
clickhouse_test_SOURCES = test.cc

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet2BackendClickHouse.pc

include $(top_builddir)/include.mk

endif
