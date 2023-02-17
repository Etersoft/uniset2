if ENABLE_OPCUA
bin_PROGRAMS = @PACKAGE@-opcua-server

OPCUA_VER=@LIBVER@

lib_LTLIBRARIES = libUniSet2OPCUAServer.la
libUniSet2OPCUAServer_la_SOURCES = OPCUAServer.cc
libUniSet2OPCUAServer_la_LDFLAGS = -version-info $(OPCUA_VER)
libUniSet2OPCUAServer_la_LIBADD = $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(SIGC_LIBS) $(POCO_LIBS) $(OPEN62541PP_LIBS)
libUniSet2OPCUAServer_la_CXXFLAGS = -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS) $(OPEN62541PP_CFLAGS)


@PACKAGE@_opcua_server_SOURCES = main.cc
@PACKAGE@_opcua_server_CXXFLAGS = -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS) $(OPEN62541PP_CFLAGS)
@PACKAGE@_opcua_server_LDADD = libUniSet2OPCUAServer.la $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(SIGC_LIBS) $(POCO_LIBS) $(OPEN62541PP_LIBS)


# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions/opcua

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = libUniSet2OPCUAServer.pc


include $(top_builddir)/include.mk
endif
