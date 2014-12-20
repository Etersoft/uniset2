if DISABLE_PGSQL

else

UPGSQL_VER=@LIBVER@

lib_LTLIBRARIES 			= libUniSet2-postgresql.la
libUniSet2_postgresql_la_LDFLAGS  = -version-info $(UPGSQL_VER)
libUniSet2_postgresql_la_SOURCES	= PostgreSQLInterface.cc DBServer_PostgreSQL.cc
libUniSet2_postgresql_la_LIBADD	= $(top_builddir)/lib/libUniSet2.la $(POSTGRESQL_LIBS)
libUniSet2_postgresql_la_CXXFLAGS = $(POSTGRESQL_CFLAGS)

bin_PROGRAMS 					= @PACKAGE@-postgresql-dbserver
@PACKAGE@_postgresql_dbserver_LDADD		= libUniSet2-postgresql.la $(top_builddir)/lib/libUniSet2.la $(POSTGRESQL_LIBS)
@PACKAGE@_postgresql_dbserver_CXXFLAGS	= $(POSTGRESQL_CFLAGS)
@PACKAGE@_postgresql_dbserver_SOURCES	= main.cc

noinst_PROGRAMS = postgresql-test
postgresql_test_LDADD   = libUniSet2-postgresql.la $(top_builddir)/lib/libUniSet2.la $(POSTGRESQL_LIBS)
postgresql_test_CXXFLAGS = $(POSTGRESQL_CFLAGS)
postgresql_test_SOURCES = test.cc

# install
devel_include_HEADERS = *.h
devel_includedir = $(includedir)/@PACKAGE@/postgresql

pkgconfigdir    = $(libdir)/pkgconfig
pkgconfig_DATA  = libUniSet2PostgreSQL.pc

endif
