if HAVE_TESTS
############################################################################
# This file is part of the UniSet library                                  #
############################################################################

#check_PROGRAMS = tests tests_with_conf
noinst_PROGRAMS = tests tests_with_conf

#umutex threadtst dlog
tests_LDADD 	= $(top_builddir)/lib/libUniSet2.la $(SIGC_LIBS) $(COMCPP_LIBS)
tests_CPPFLAGS	= -I$(top_builddir)/include $(SIGC_CFLAGS) $(COMCPP_CFLAGS)
tests_SOURCES = tests.cc \
test_passivetimer.cc \
test_hourglass.cc \
test_delaytimer.cc \
test_unixml.cc \
test_sscanf_hex.cc \
test_callbacktimer.cc \
test_trigger.cc \
test_triggerOR.cc \
test_triggerAND.cc \
test_triggerOUT.cc \
test_pulse.cc \
test_modbustypes.cc \
test_utypes.cc \
test_mutex.cc

tests_with_conf_LDADD   = $(top_builddir)/lib/libUniSet2.la
tests_with_conf_CPPFLAGS = -I$(top_builddir)/include
tests_with_conf_SOURCES = tests_with_conf.cc \
test_conftest.cc \
test_ui.cc \
test_iorfile.cc \
test_messagetype.cc 
     
# umutex_SOURCES 	= umutex.cc
# umutex_LDADD 	= $(top_builddir)/lib/libUniSet2.la $(COMCPP_LIBS)
# umutex_CPPFLAGS	= -I$(top_builddir)/include $(COMCPP_CFLAGS)
# 
# threadtst_SOURCES = threadtst.cc
# threadtst_LDADD 	= $(top_builddir)/lib/libUniSet2.la ${SIGC_LIBS} $(COMCPP_LIBS)
# threadtst_CPPFLAGS	= -I$(top_builddir)/include ${SIGC_CFLAGS} $(COMCPP_CFLAGS)
# 
# dlog_SOURCES = dlog.cc
# dlog_LDADD 	= $(top_builddir)/lib/libUniSet2.la ${SIGC_LIBS} $(COMCPP_LIBS)
# dlog_CPPFLAGS = -I$(top_builddir)/include ${SIGC_CFLAGS} $(COMCPP_CFLAGS)
# 

include $(top_builddir)/testsuite/testsuite-common.mk

check-local: atconfig package.m4 $(TESTSUITE)
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)

clean-local:
	test ! -f '$(TESTSUITE)'|| $(SHELL) '$(TESTSUITE)' --clean
	rm -rf $(CLEANFILES)
	rm -rf $(COVERAGE_REPORT_DIR) .coverage* *.gcno

if HAVE_COVERAGE
include $(top_builddir)/testsuite/testsuite-coverage-common.mk
endif

endif
