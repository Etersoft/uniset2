if HAVE_TESTS
if ENABLE_JSCRIPT

noinst_PROGRAMS = tests-with-sm
if ENABLE_OPCUA
noinst_PROGRAMS += tests-opcua
endif

tests_with_sm_SOURCES = tests.cc test_jsproxy.cc ModbusTCPTestServer.cc
tests_with_sm_LDADD  = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(top_builddir)/extensions/JScript/libUniSet2JScript.la \
	$(SIGC_LIBS) $(POCO_LIBS) $(QUICKJS_LIBS)
tests_with_sm_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/JScript \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS) $(QUICKJS_CFLAGS)

if ENABLE_OPCUA
tests_opcua_SOURCES = tests_opcua.cc tests_opcua_client.cc OPCUATestServer.cc
tests_opcua_LDADD = $(top_builddir)/extensions/JScript/libUniSet2JScriptOpcua.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(top_builddir)/lib/libUniSet2.la $(SIGC_LIBS) $(POCO_LIBS) $(QUICKJS_LIBS) $(OPEN62541_LIBS) $(OPEN62541PP_LIBS)
tests_opcua_CPPFLAGS = -DJS_OPCUA_ENABLED -I$(top_builddir)/include -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory \
	-I$(top_builddir)/extensions/JScript $(SIGC_CFLAGS) $(POCO_CFLAGS) $(QUICKJS_CFLAGS) $(OPEN62541_CFLAGS) $(OPEN62541PP_CFLAGS)
endif

include $(top_builddir)/testsuite/testsuite-common.mk

check-local: atconfig package.m4 $(TESTSUITE) jscript-tests.at
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)

clean-local:
	rm -rf $(CLEANFILES)
	rm -rf $(COVERAGE_REPORT_DIR)

include $(top_builddir)/include.mk

endif
endif
