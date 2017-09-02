if DISABLE_SQLITE

else

bin_PROGRAMS = @PACKAGE@-logdb
@PACKAGE@_logdb_LDADD = $(top_builddir)/extensions/DBServer-SQLite/libUniSet2-sqlite.la $(top_builddir)/lib/libUniSet2.la
@PACKAGE@_logdb_CXXFLAGS = $(SQLITE3_CFLAGS) -I$(top_builddir)/extensions/DBServer-SQLite
@PACKAGE@_logdb_SOURCES = LogDB.cc main.cc

include $(top_builddir)/include.mk

endif
