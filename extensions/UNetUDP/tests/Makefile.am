if HAVE_TESTS

noinst_PROGRAMS = tests-with-sm tests-multicast-with-sm urecv-perf-test

tests_with_sm_SOURCES   = tests_with_sm.cc test_unetudp.cc
tests_with_sm_LDADD     = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/UNetUDP/libUniSet2UNetUDP.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(SIGC_LIBS) $(POCO_LIBS)
tests_with_sm_CPPFLAGS  = -I$(top_srcdir)/include -I$(top_srcdir)/extensions/include \
	-I$(top_srcdir)/extensions/UNetUDP \
	-I$(top_srcdir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS)

tests_multicast_with_sm_SOURCES   = tests_multicast_with_sm.cc test_unetmulticast.cc test_unetudp_multicast.cc
tests_multicast_with_sm_LDADD     = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/UNetUDP/libUniSet2UNetUDP.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(SIGC_LIBS) $(POCO_LIBS)
tests_multicast_with_sm_CPPFLAGS  = -I$(top_srcdir)/include -I$(top_srcdir)/extensions/include \
	-I$(top_srcdir)/extensions/UNetUDP \
	-I$(top_srcdir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS)

urecv_perf_test_SOURCES   = urecv_perf_test.cc
urecv_perf_test_LDADD     = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/UNetUDP/libUniSet2UNetUDP.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(SIGC_LIBS) $(POCO_LIBS)
urecv_perf_test_CPPFLAGS  = -I$(top_srcdir)/include -I$(top_srcdir)/extensions/include \
	-I$(top_srcdir)/extensions/UNetUDP \
	-I$(top_srcdir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS)


include $(top_builddir)/testsuite/testsuite-common.mk

check-local: atconfig package.m4 $(TESTSUITE) unetudp-tests.at
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)

clean-local:
	rm -rf $(CLEANFILES)
	rm -rf $(COVERAGE_REPORT_DIR)

endif
