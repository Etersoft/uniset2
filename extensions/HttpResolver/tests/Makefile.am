if ENABLE_REST_API
if HAVE_TESTS

noinst_PROGRAMS = run_test_uresolver

run_test_uresolver_SOURCES = run_test_uresolver.cc test_uresolver.cc TestObject.cc
run_test_uresolver_LDADD  = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la $(SIGC_LIBS) $(POCO_LIBS)
run_test_uresolver_CPPFLAGS = -I$(top_builddir)/include -I$(top_builddir)/extensions/include $(SIGC_CFLAGS) $(POCO_CFLAGS)

include $(top_builddir)/testsuite/testsuite-common.mk

check-local: atconfig package.m4 $(TESTSUITE) uresolver-tests.at
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)

clean-local:
	rm -rf $(CLEANFILES)
	rm -rf $(COVERAGE_REPORT_DIR)

include $(top_builddir)/include.mk

endif
endif
