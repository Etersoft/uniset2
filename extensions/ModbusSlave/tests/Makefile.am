if HAVE_TESTS

noinst_PROGRAMS = tests-with-sm mbslave-perf-test

tests_with_sm_SOURCES   = tests_with_sm.cc test_mbslave.cc
tests_with_sm_LDADD	 = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/ModbusSlave/libUniSet2MBSlave.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(SIGC_LIBS) $(POCO_LIBS)
tests_with_sm_CPPFLAGS  = -I$(top_srcdir)/include -I$(top_srcdir)/extensions/include \
	-I$(top_srcdir)/extensions/ModbusSlave \
	-I$(top_srcdir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS)


mbslave_perf_test_SOURCES   = mbslave_perf_test.cc
mbslave_perf_test_LDADD	 = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/ModbusSlave/libUniSet2MBSlave.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(SIGC_LIBS) $(POCO_LIBS)
mbslave_perf_test_CPPFLAGS  = -I$(top_srcdir)/include -I$(top_srcdir)/extensions/include \
	-I$(top_srcdir)/extensions/ModbusSlave \
	-I$(top_srcdir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS)

include $(top_builddir)/testsuite/testsuite-common.mk

check-local: atconfig package.m4 $(TESTSUITE) mbslave-tests.at
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)
	
clean-local:
	rm -rf $(CLEANFILES)
	rm -rf $(COVERAGE_REPORT_DIR) 

include $(top_builddir)/include.mk

endif
