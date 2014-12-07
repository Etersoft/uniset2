SUBDIR=SMemoryTest

if  HAVE_TESTS
noinst_PROGRAMS = tests tests_with_conf tests_with_sm

tests_SOURCES   = tests.cc test_digitalfilter.cc test_vtypes.cc
tests_LDADD     = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la
tests_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include

tests_with_conf_SOURCES   = tests_with_conf.cc test_calibration.cc test_iobase.cc
tests_with_conf_LDADD     = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la
tests_with_conf_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include

tests_with_sm_SOURCES   = tests_with_sm.cc test_ui.cc
tests_with_sm_LDADD     = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la $(SIGC_LIBS) $(COMCPP_LIBS)
tests_with_sm_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)  $(COMCPP_CFLAGS)


include $(top_builddir)/testsuite/testsuite-common.mk

check-local: atconfig package.m4 $(TESTSUITE)
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)
    
clean-local:
	test ! -f '$(TESTSUITE)'|| $(SHELL) '$(TESTSUITE)' --clean
	rm -rf $(CLEANFILES)
	rm -rf $(COVERAGE_REPORT_DIR)

if HAVE_COVERAGE
include $(top_builddir)/testsuite/testsuite-coverage-common.mk
endif

endif
