bin_PROGRAMS 			= @PACKAGE@-smdbserver

USMDB_VER=@LIBVER@

lib_LTLIBRARIES 			= libUniSetSMDBServer.la
libUniSetSMDBServer_la_LDFLAGS  = -version-info $(USMDB_VER) 
libUniSetSMDBServer_la_LIBADD		= $(top_builddir)/lib/libUniSet.la \
									$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSetExtensions.la \
									$(top_builddir)/Utilities/DBServer-MySQL/libUniSet-mysql.la \
									$(SIGC_LIBS)
libUniSetSMDBServer_la_CXXFLAGS	= -I$(top_builddir)/extensions/include \
									-I$(top_builddir)/extensions/SharedMemory \
									-I$(top_builddir)/Utilities/DBServer-MySQL \
									$(SIGC_CFLAGS)
libUniSetSMDBServer_la_SOURCES 	= SMDBServer.cc

@PACKAGE@_smdbserver_SOURCES 	= main.cc
@PACKAGE@_smdbserver_LDADD 	= libUniSetSMDBServer.la $(top_builddir)/lib/libUniSet.la \
									$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSetExtensions.la \
									$(top_builddir)/Utilities/DBServer-MySQL/libUniSet-mysql.la \
									$(SIGC_LIBS)
@PACKAGE@_smdbserver_CXXFLAGS	= -I$(top_builddir)/extensions/include \
									-I$(top_builddir)/extensions/SharedMemory \
									-I$(top_builddir)/Utilities/DBServer-MySQL \
									$(SIGC_CFLAGS)

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSetSMDBServer.pc

all-local:
	ln -sf ../SMDBServer/$(devel_include_HEADERS) ../include
