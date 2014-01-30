if DISABLE_SQLITE

else

USQLITE_VER=@LIBVER@

lib_LTLIBRARIES = libUniSet2-sqlite.la
libUniSet2_sqlite_la_LDFLAGS = -version-info $(USQLITE_VER)
libUniSet2_sqlite_la_SOURCES = SQLiteInterface.cc DBServer_SQLite.cc
libUniSet2_sqlite_la_LIBADD = $(top_builddir)/lib/libUniSet2.la $(SQLITE3_LIBS)
libUniSet2_sqlite_la_CXXFLAGS = $(SQLITE3_CFLAGS)

bin_PROGRAMS = @PACKAGE@-sqlite-dbserver
@PACKAGE@_sqlite_dbserver_LDADD = libUniSet2-sqlite.la $(top_builddir)/lib/libUniSet2.la
@PACKAGE@_sqlite_dbserver_SOURCES = main.cc

noinst_PROGRAMS = sqlite-test
sqlite_test_LDADD = libUniSet2-sqlite.la $(top_builddir)/lib/libUniSet2.la
sqlite_test_SOURCES = test.cc

# install
devel_include_HEADERS = *.h
devel_includedir = $(includedir)/@PACKAGE@/sqlite

pkgconfigdir    = $(libdir)/pkgconfig
pkgconfig_DATA  = libUniSet2SQLite.pc

endif
                                