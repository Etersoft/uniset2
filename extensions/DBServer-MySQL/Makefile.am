if DISABLE_MYSQL

else

UMYSQL_VER=@LIBVER@

lib_LTLIBRARIES 			= libUniSet-mysql.la
libUniSet_mysql_la_LDFLAGS  = -version-info $(UMYSQL_VER)
libUniSet_mysql_la_SOURCES	= DBInterface.cc DBServer_MySQL.cc
libUniSet_mysql_la_LIBADD	= $(top_builddir)/lib/libUniSet.la -lmysqlclient

bin_PROGRAMS 					= uniset-mysql-dbserver
uniset_mysql_dbserver_LDADD		= libUniSet-mysql.la $(top_builddir)/lib/libUniSet.la
uniset_mysql_dbserver_SOURCES	= main.cc

# install
devel_include_HEADERS = *.h
devel_includedir = $(includedir)/@PACKAGE@/mysql

endif
