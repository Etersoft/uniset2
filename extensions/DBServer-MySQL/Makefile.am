if DISABLE_MYSQL

else

UMYSQL_VER=@LIBVER@

lib_LTLIBRARIES 			= libUniSet2-mysql.la
libUniSet2_mysql_la_LDFLAGS  = -version-info $(UMYSQL_VER)
libUniSet2_mysql_la_SOURCES	= DBInterface.cc DBServer_MySQL.cc
libUniSet2_mysql_la_LIBADD	= $(top_builddir)/lib/libUniSet2.la -lmysqlclient

bin_PROGRAMS 					= @PACKAGE@-mysql-dbserver
@PACKAGE@_mysql_dbserver_LDADD		= libUniSet2-mysql.la $(top_builddir)/lib/libUniSet2.la
@PACKAGE@_mysql_dbserver_SOURCES	= main.cc

# install
devel_include_HEADERS = *.h
devel_includedir = $(includedir)/@PACKAGE@/mysql

pkgconfigdir    = $(libdir)/pkgconfig
pkgconfig_DATA  = libUniSet2MySQL.pc

endif
