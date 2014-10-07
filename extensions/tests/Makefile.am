SUBDIR=SMemoryTest

#noinst_PROGRAMS = tests tests_with_conf tests_with_sm
check_PROGRAMS = tests tests_with_conf tests_with_sm

TESTS_ENVIRONMENT=$(CATCH_TESTS_ENVIRONMENT)
TESTS=tests tests_with_conf.sh tests_with_sm.sh

tests_SOURCES   = tests.cc digitalfilter.cc
tests_LDADD     = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la
tests_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include -I$(includedir)/Catch

tests_with_conf_SOURCES   = tests_with_conf.cc  calibration.cc
tests_with_conf_LDADD     = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la
tests_with_conf_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include -I$(includedir)/Catch

tests_with_sm_SOURCES   = tests_with_sm.cc ui.cc
tests_with_sm_LDADD     = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la $(SIGC_LIBS) $(COMCPP_LIBS)
tests_with_sm_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include -I$(includedir)/Catch \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)  $(COMCPP_CFLAGS)

clean-local:	
	rm -rf *.log
