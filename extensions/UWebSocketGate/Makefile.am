if ENABLE_REST_API
bin_PROGRAMS = @PACKAGE@-wsgate

WSGATE_VER=@LIBVER@

lib_LTLIBRARIES = libUniSet2UWebSocketGate.la
libUniSet2UWebSocketGate_la_SOURCES = UWebSocketGate.cc
libUniSet2UWebSocketGate_la_LDFLAGS = -version-info $(WSGATE_VER)
libUniSet2UWebSocketGate_la_LIBADD = $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(SIGC_LIBS) $(POCO_LIBS)
libUniSet2UWebSocketGate_la_CXXFLAGS = -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS)


@PACKAGE@_wsgate_SOURCES = main.cc
@PACKAGE@_wsgate_CXXFLAGS = -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS)
@PACKAGE@_wsgate_LDADD = libUniSet2UWebSocketGate.la $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(SIGC_LIBS) $(POCO_LIBS)


# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions/wsgate

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = libUniSet2UWebSocketGate.pc


include $(top_builddir)/include.mk
endif
