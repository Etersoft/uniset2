if ENABLE_CLICKHOUSE
if HAVE_TESTS

noinst_PROGRAMS = tests

tests_SOURCES = test_ClickHouseTagsConfig.cc tests.cc
tests_LDADD	 = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/Backend-ClickHouse/libUniSet2BackendClickHouse.la \
	$(SIGC_LIBS) $(POCO_LIBS) -lpthread
tests_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/Backend-ClickHouse $(SIGC_CFLAGS) $(POCO_CFLAGS)

include $(top_builddir)/testsuite/testsuite-common.mk

check-local: atconfig package.m4 $(TESTSUITE) backend-clickhouse-tests.at
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)

clean-local:
	rm -rf $(CLEANFILES)
	rm -rf $(COVERAGE_REPORT_DIR)

include $(top_builddir)/include.mk

endif
endif
