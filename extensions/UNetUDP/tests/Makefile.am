if HAVE_TESTS

noinst_PROGRAMS = tests-with-sm

tests_with_sm_SOURCES   = tests_with_sm.cc test_unetudp.cc
tests_with_sm_LDADD     = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/UNetUDP/libUniSet2UNetUDP.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(SIGC_LIBS) $(COMCPP_LIBS) $(COV_LIBS)
tests_with_sm_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/UNetUDP \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)  $(COMCPP_CFLAGS) $(COV_CFLAGS)

include $(top_builddir)/testsuite/testsuite-common.mk

check-local: atconfig package.m4 $(TESTSUITE) unetudp-tests.at
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)

clean-local:
	rm -rf $(CLEANFILES)
	rm -rf $(COVERAGE_REPORT_DIR)
	rm -f *.gcno *.gcda

if HAVE_COVERAGE
# COVERAGE_EXCLUDE_FILES='include/*'
include $(top_builddir)/testsuite/testsuite-coverage-common.mk
endif

endif
