if ENABLE_PGSQL
UPGSQL_VER=@LIBVER@

lib_LTLIBRARIES              = libUniSet2-pgsql.la
libUniSet2_pgsql_la_LDFLAGS  = -version-info $(UPGSQL_VER)
libUniSet2_pgsql_la_SOURCES  = PostgreSQLInterface.cc DBServer_PostgreSQL.cc
libUniSet2_pgsql_la_LIBADD   = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la $(PGSQL_LIBS)
libUniSet2_pgsql_la_CXXFLAGS = -std=c++17 -I$(top_builddir)/extensions/SharedMemory $(PGSQL_CFLAGS)

bin_PROGRAMS                      = @PACKAGE@-pgsql-dbserver
@PACKAGE@_pgsql_dbserver_LDADD    = libUniSet2-pgsql.la $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la $(PGSQL_LIBS)
@PACKAGE@_pgsql_dbserver_CXXFLAGS = -std=c++17 -I$(top_builddir)/extensions/SharedMemory $(PGSQL_CFLAGS)
@PACKAGE@_pgsql_dbserver_SOURCES  = main.cc

noinst_PROGRAMS     = pgsql-test
pgsql_test_LDADD    = libUniSet2-pgsql.la $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la $(PGSQL_LIBS)
pgsql_test_CXXFLAGS = -std=c++17 -I$(top_builddir)/extensions/SharedMemory $(PGSQL_CFLAGS)
pgsql_test_SOURCES  = test.cc

# install
devel_include_HEADERS = *.h
devel_includedir = $(includedir)/@PACKAGE@/extensions/pgsql

pkgconfigdir    = $(libdir)/pkgconfig
pkgconfig_DATA  = libUniSet2PostgreSQL.pc

include $(top_builddir)/include.mk

endif
