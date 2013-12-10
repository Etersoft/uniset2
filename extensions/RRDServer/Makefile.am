bin_PROGRAMS 			= @PACKAGE@-rrdserver

RRD_VER=@LIBVER@

lib_LTLIBRARIES 			= libUniSetRRDServer.la
libUniSetRRDServer_la_LDFLAGS  = -version-info $(RRD_VER) 
libUniSetRRDServer_la_LIBADD		= $(top_builddir)/lib/libUniSet.la \
									$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSetExtensions.la \
									$(SIGC_LIBS) $(RRD_LIBS)
libUniSetRRDServer_la_CXXFLAGS	= -I$(top_builddir)/extensions/include \
									-I$(top_builddir)/extensions/SharedMemory \
									$(SIGC_CFLAGS) $(RRD_CFLAGS)
libUniSetRRDServer_la_SOURCES 	= RRDServer.cc

@PACKAGE@_rrdserver_SOURCES 	= main.cc
@PACKAGE@_rrdserver_LDADD 	= libUniSetRRDServer.la $(top_builddir)/lib/libUniSet.la \
									$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSetExtensions.la \
									$(SIGC_LIBS) $(RRD_LIBS)
@PACKAGE@_rrdserver_CXXFLAGS	= -I$(top_builddir)/extensions/include \
									-I$(top_builddir)/extensions/SharedMemory \
									$(SIGC_CFLAGS) $(RRD_CFLAGS)

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSetRRDServer.pc

all-local:
	ln -sf ../RRDServer/$(devel_include_HEADERS) ../include
