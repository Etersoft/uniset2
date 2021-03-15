if ENABLE_REST_API
bin_PROGRAMS = @PACKAGE@-wsgate
@PACKAGE@_wsgate_CXXFLAGS = -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS)
@PACKAGE@_wsgate_LDADD = $(top_builddir)/lib/libUniSet2.la \
    $(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
    $(top_builddir)/extensions/lib/libUniSet2Extensions.la \
    $(SIGC_LIBS) $(POCO_LIBS)
@PACKAGE@_wsgate_SOURCES = UWebSocketGate.cc main.cc

include $(top_builddir)/include.mk
endif
