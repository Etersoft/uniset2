SUBDIRS=SMemoryTest MBSlaveTest MQPerfTest

if  HAVE_TESTS
noinst_PROGRAMS = tests tests_with_conf tests_with_sm sm_perf_test

tests_SOURCES   = tests.cc test_digitalfilter.cc test_vtypes.cc
tests_LDADD	 = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la
tests_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include

tests_with_conf_SOURCES   = tests_with_conf.cc test_calibration.cc test_iobase.cc
tests_with_conf_LDADD	 = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la
tests_with_conf_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include

tests_with_sm_SOURCES   = tests_with_sm.cc test_ui.cc test_iobase_with_sm.cc
tests_with_sm_LDADD	 = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la $(SIGC_LIBS) $(COMCPP_LIBS)
tests_with_sm_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(COMCPP_CFLAGS)


sm_perf_test_SOURCES   = sm_perf_test.cc
sm_perf_test_LDADD	 = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la $(SIGC_LIBS) -lpthread
sm_perf_test_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(COMCPP_CFLAGS)


include $(top_builddir)/testsuite/testsuite-common.mk

check-local: atconfig package.m4 $(TESTSUITE)
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)
	
clean-local:
	test ! -f '$(TESTSUITE)'|| $(SHELL) '$(TESTSUITE)' --clean
	rm -rf $(CLEANFILES)

if HAVE_COVERAGE
COVERAGE_DIRS=$(abs_srcdir) $(top_builddir)/src/Interfaces $(top_builddir)/extensions/lib $(top_builddir)/extensions/SharedMemory
include $(top_builddir)/testsuite/testcoverage-common.mk
endif

include $(top_builddir)/include.mk

endif
