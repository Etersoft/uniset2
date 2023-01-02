if ENABLE_OPCUA
bin_PROGRAMS = @PACKAGE@-opcuagate

OPCUA_VER=@LIBVER@

lib_LTLIBRARIES = libUniSet2OPCUAGate.la
libUniSet2OPCUAGate_la_SOURCES = OPCUAGate.cc
libUniSet2OPCUAGate_la_LDFLAGS = -version-info $(OPCUA_VER)
libUniSet2OPCUAGate_la_LIBADD = $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(SIGC_LIBS) $(POCO_LIBS) $(OPEN62541PP_LIBS)
libUniSet2OPCUAGate_la_CXXFLAGS = -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS) $(OPEN62541PP_CFLAGS)


@PACKAGE@_opcuagate_SOURCES = main.cc
@PACKAGE@_opcuagate_CXXFLAGS = -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS) $(OPEN62541PP_CFLAGS)
@PACKAGE@_opcuagate_LDADD = libUniSet2OPCUAGate.la $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(SIGC_LIBS) $(POCO_LIBS) $(OPEN62541PP_LIBS)


# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions/opcuagate

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = libUniSet2OPCUAGate.pc


include $(top_builddir)/include.mk
endif
