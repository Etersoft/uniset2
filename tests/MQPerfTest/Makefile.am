noinst_PROGRAMS = mq-test
mq_test_LDADD = $(top_builddir)/lib/libUniSet2.la $(SIGC_LIBS) $(POCO_LIBS) -lpthread
mq_test_CPPFLAGS = -I$(top_builddir)/include -I$(top_builddir)/extensions/include $(SIGC_CFLAGS) $(POCO_CFLAGS)
mq_test_SOURCES = mq-test.cc
