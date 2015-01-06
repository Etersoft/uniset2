if HAVE_TESTS

noinst_PROGRAMS = tests-with-sm

tests_with_sm_SOURCES   = tests_with_sm.cc test_mbtcpmaster.cc test_mbtcpmultimaster.cc
tests_with_sm_LDADD     = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/ModbusMaster/libUniSet2MBTCPMaster.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(SIGC_LIBS) $(COMCPP_LIBS)
tests_with_sm_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/ModbusMaster \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)  $(COMCPP_CFLAGS)

include $(top_builddir)/testsuite/testsuite-common.mk

check-local: atconfig package.m4 $(TESTSUITE) mbmaster-tests.at
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)

clean-local:
	rm -rf $(CLEANFILES)
	rm -rf $(COVERAGE_REPORT_DIR)

if HAVE_COVERAGE
include $(top_builddir)/testsuite/testsuite-coverage-common.mk
endif

endif
