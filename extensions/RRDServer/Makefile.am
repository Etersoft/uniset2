if DISABLE_RRD

else

bin_PROGRAMS = @PACKAGE@-rrdserver

RRD_VER=@LIBVER@

lib_LTLIBRARIES = libUniSet2RRDServer.la
libUniSet2RRDServer_la_LDFLAGS  = -version-info $(RRD_VER)
libUniSet2RRDServer_la_LIBADD		= $(top_builddir)/lib/libUniSet2.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
									$(SIGC_LIBS) $(RRD_LIBS)
libUniSet2RRDServer_la_CXXFLAGS	= -I$(top_builddir)/extensions/include \
									-I$(top_builddir)/extensions/SharedMemory \
									$(SIGC_CFLAGS) $(RRD_CFLAGS)
libUniSet2RRDServer_la_SOURCES 	= RRDServer.cc

@PACKAGE@_rrdserver_SOURCES 	= main.cc
@PACKAGE@_rrdserver_LDADD 	= libUniSet2RRDServer.la $(top_builddir)/lib/libUniSet2.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
									$(SIGC_LIBS) $(RRD_LIBS)
@PACKAGE@_rrdserver_CXXFLAGS	= -I$(top_builddir)/extensions/include \
									-I$(top_builddir)/extensions/SharedMemory \
									$(SIGC_CFLAGS) $(RRD_CFLAGS)

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet2RRDServer.pc

all-local:
	ln -sf ../RRDServer/$(devel_include_HEADERS) ../include

endif