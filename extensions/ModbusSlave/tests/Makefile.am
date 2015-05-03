if HAVE_TESTS

noinst_PROGRAMS = tests-with-sm

tests_with_sm_SOURCES   = tests_with_sm.cc test_mbslave.cc
tests_with_sm_LDADD	 = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/ModbusSlave/libUniSet2MBSlave.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(SIGC_LIBS) $(COMCPP_LIBS)
tests_with_sm_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/ModbusSlave \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(COMCPP_CFLAGS)

#$(top_builddir)/extensions/lib/libUniSet2Extensions.la:
#	cd $(top_builddir)/extensions/lib/ && make

#$(top_builddir)/extensions/ModbusSlave/libUniSet2MBSlave.la:
#	cd $(top_builddir)/extensions/ModbusSlave/ && make

include $(top_builddir)/testsuite/testsuite-common.mk

check-local: atconfig package.m4 $(TESTSUITE) mbslave-tests.at
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)
	
clean-local:
	rm -rf $(CLEANFILES)
	rm -rf $(COVERAGE_REPORT_DIR) 

include $(top_builddir)/include.mk

endif
