if HAVE_TESTS

noinst_PROGRAMS = run_test_opcua_exchange

run_test_opcua_exchange_SOURCES = run_test_opcua_exchange.cc test_opcua_exchange.cc OPCUATestServer.cc
run_test_opcua_exchange_LDADD = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/OPCUAExchange/libUniSet2OPCUAExchange.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(SIGC_LIBS) $(POCO_LIBS) $(OPEN62541_LIBS) $(OPEN62541PP_LIBS)
run_test_opcua_exchange_CPPFLAGS = -I$(top_builddir)/include -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/OPCUAExchange \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS) $(OPEN62541_CFLAGS) $(OPEN62541PP_CFLAGS)

include $(top_builddir)/testsuite/testsuite-common.mk

check-local: atconfig package.m4 $(TESTSUITE) opcua-exchange-tests.at
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)

clean-local:
	rm -rf $(CLEANFILES)
	rm -rf $(COVERAGE_REPORT_DIR)

include $(top_builddir)/include.mk

endif
