if HAVE_TESTS

noinst_PROGRAMS = run_test_mbtcpmaster run_test_mbtcpmultimaster

run_test_mbtcpmaster_SOURCES   = run_test_mbtcpmaster.cc test_mbtcpmaster.cc MBTCPTestServer.cc
run_test_mbtcpmaster_LDADD     = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/ModbusMaster/libUniSet2MBTCPMaster.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(SIGC_LIBS) $(COMCPP_LIBS)
run_test_mbtcpmaster_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/ModbusMaster \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(COMCPP_CFLAGS)

run_test_mbtcpmultimaster_SOURCES   = run_test_mbtcpmultimaster.cc test_mbtcpmultimaster.cc MBTCPTestServer.cc
run_test_mbtcpmultimaster_LDADD     = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/ModbusMaster/libUniSet2MBTCPMaster.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(SIGC_LIBS) $(COMCPP_LIBS)
run_test_mbtcpmultimaster_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/ModbusMaster \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(COMCPP_CFLAGS)

include $(top_builddir)/testsuite/testsuite-common.mk

check-local: atconfig package.m4 $(TESTSUITE) mbmaster-tests.at
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)

clean-local:
	rm -rf $(CLEANFILES)
	rm -rf $(COVERAGE_REPORT_DIR)

endif
