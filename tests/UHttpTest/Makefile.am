if ENABLE_REST_API
noinst_PROGRAMS = uhttp-test
uhttp_test_LDADD = $(top_builddir)/lib/libUniSet2.la $(SIGC_LIBS) $(POCO_LIBS)
uhttp_test_CPPFLAGS = -I$(top_builddir)/include -I$(top_builddir)/extensions/include $(SIGC_CFLAGS) $(POCO_CFLAGS)
uhttp_test_SOURCES = uhttp-test.cc
endif