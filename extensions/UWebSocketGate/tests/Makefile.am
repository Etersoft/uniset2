if HAVE_TESTS
if ENABLE_UWEBSOCKETGATE

noinst_PROGRAMS = tests-with-sm

tests_with_sm_SOURCES   = tests_with_sm.cc test_uwebsocketgate.cc $(top_builddir)/extensions/UWebSocketGate/UWebSocketGate.cc
tests_with_sm_LDADD	 = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(SIGC_LIBS) $(POCO_LIBS)
tests_with_sm_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/UWebSocketGate \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS)


include $(top_builddir)/testsuite/testsuite-common.mk

check-local: atconfig package.m4 $(TESTSUITE) uwebsocketgate-tests.at
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)
	
clean-local:
	rm -rf $(CLEANFILES)
	rm -rf $(COVERAGE_REPORT_DIR) 

include $(top_builddir)/include.mk

endif
endif
