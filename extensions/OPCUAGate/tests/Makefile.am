if HAVE_TESTS
if ENABLE_OPCUA

noinst_PROGRAMS = tests-with-sm

tests_with_sm_SOURCES   = tests_with_sm.cc test_opcuagate.cc $(top_builddir)/extensions/OPCUAGate/OPCUAGate.cc
tests_with_sm_LDADD	 = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(top_builddir)/extensions/OPCUAGate/libUniSet2OPCUAGate.la \
	$(SIGC_LIBS) $(POCO_LIBS) $(OPEN62541PP_LIBS) $(OPEN62541_LIBS)
tests_with_sm_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/OPCUAGate \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS) $(OPEN62541PP_CFLAGS) $(OPEN62541_CFLAGS)

include $(top_builddir)/testsuite/testsuite-common.mk

check-local: atconfig package.m4 $(TESTSUITE) opcua-tests.at
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)

clean-local:
	rm -rf $(CLEANFILES)
	rm -rf $(COVERAGE_REPORT_DIR)

include $(top_builddir)/include.mk

endif
endif
