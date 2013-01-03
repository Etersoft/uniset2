#if DISABLE_SQLITE

#else

USQLITE_VER=@LIBVER@

lib_LTLIBRARIES = libUniSet-sqlite.la
libUniSet_sqlite_la_LDFLAGS = -version-info $(USQLITE_VER)
libUniSet_sqlite_la_SOURCES = SQLiteInterface.cc DBServer_SQLite.cc
libUniSet_sqlite_la_LIBADD = $(top_builddir)/lib/libUniSet.la $(SQLITE3_LIBS)
libUniSet_sqlite_la_CXXFLAGS = $(SQLITE3_CFLAGS)

bin_PROGRAMS = uniset-sqlite-dbserver
uniset_sqlite_dbserver_LDADD = libUniSet-sqlite.la $(top_builddir)/lib/libUniSet.la
uniset_sqlite_dbserver_SOURCES = main.cc

noinst_PROGRAMS = sqlite-test
sqlite_test_LDADD = libUniSet-sqlite.la $(top_builddir)/lib/libUniSet.la
sqlite_test_SOURCES = test.cc

include $(top_builddir)/conf/setting.mk

# install
devel_include_HEADERS = *.h
devel_includedir = $(includedir)/@PACKAGE@/sqlite

pkgconfigdir    = $(libdir)/pkgconfig
pkgconfig_DATA  = libUniSetSQLite.pc

#endif
