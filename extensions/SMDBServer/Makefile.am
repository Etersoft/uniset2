bin_PROGRAMS 			= @PACKAGE@-smdbserver

USMDB_VER=@LIBVER@

lib_LTLIBRARIES 			= libUniSet2SMDBServer.la
libUniSet2SMDBServer_la_LDFLAGS  = -version-info $(USMDB_VER)
libUniSet2SMDBServer_la_LIBADD		= $(top_builddir)/lib/libUniSet2.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
									$(top_builddir)/Utilities/DBServer-MySQL/libUniSet2-mysql.la \
									$(SIGC_LIBS)
libUniSet2SMDBServer_la_CXXFLAGS	= -I$(top_builddir)/extensions/include \
									-I$(top_builddir)/extensions/SharedMemory \
									-I$(top_builddir)/Utilities/DBServer-MySQL \
									$(SIGC_CFLAGS)
libUniSet2SMDBServer_la_SOURCES 	= SMDBServer.cc

@PACKAGE@_smdbserver_SOURCES 	= main.cc
@PACKAGE@_smdbserver_LDADD 	= libUniSet2SMDBServer.la $(top_builddir)/lib/libUniSet2.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
									$(top_builddir)/Utilities/DBServer-MySQL/libUniSet2-mysql.la \
									$(SIGC_LIBS)
@PACKAGE@_smdbserver_CXXFLAGS	= -I$(top_builddir)/extensions/include \
									-I$(top_builddir)/extensions/SharedMemory \
									-I$(top_builddir)/Utilities/DBServer-MySQL \
									$(SIGC_CFLAGS)

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet2SMDBServer.pc

#all-local:
#	ln -sf ../SMDBServer/$(devel_include_HEADERS) ../include

include $(top_builddir)/include.mk
