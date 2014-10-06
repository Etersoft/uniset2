############################################################################
# This file is part of the UniSet library                                  #
############################################################################

check_PROGRAMS = tests tests_with_conf

#TESTS_ENVIRONMENT = CATCH_TEST_ARGS="$(CATCH_TEST_ARGS)" ./test-run.sh
TESTS = tests tests_with_conf.sh
EXTRA_DIR=$(TESTS)

#umutex sscanf_hexthreadtst dlog

tests_SOURCES = tests.cc passivetimer.cc hourglass.cc delaytimer.cc unixml.cc
tests_LDADD 	= $(top_builddir)/lib/libUniSet2.la
tests_CPPFLAGS	= -I$(top_builddir)/include -I$(includedir)/Catch

tests_with_conf_SOURCES = tests_with_conf.cc conftest.cc ui.cc
tests_with_conf_LDADD   = $(top_builddir)/lib/libUniSet2.la
tests_with_conf_CPPFLAGS = -I$(top_builddir)/include -I$(includedir)/Catch
     
# ui_SOURCES 	= ui.cc
# ui_LDADD 	= $(top_builddir)/lib/libUniSet2.la
# ui_CPPFLAGS	= -I$(top_builddir)/include
# 
# umutex_SOURCES 	= umutex.cc
# umutex_LDADD 	= $(top_builddir)/lib/libUniSet2.la $(COMCPP_LIBS)
# umutex_CPPFLAGS	= -I$(top_builddir)/include $(COMCPP_CFLAGS)
# 
# sscanf_hex_SOURCES 	= sscanf_hex.cc
# 
# threadtst_SOURCES = threadtst.cc
# threadtst_LDADD 	= $(top_builddir)/lib/libUniSet2.la ${SIGC_LIBS} $(COMCPP_LIBS)
# threadtst_CPPFLAGS	= -I$(top_builddir)/include ${SIGC_CFLAGS} $(COMCPP_CFLAGS)
# 
# dlog_SOURCES = dlog.cc
# dlog_LDADD 	= $(top_builddir)/lib/libUniSet2.la ${SIGC_LIBS} $(COMCPP_LIBS)
# dlog_CPPFLAGS = -I$(top_builddir)/include ${SIGC_CFLAGS} $(COMCPP_CFLAGS)
# 

testdir=$(top_builddir)/tests

#check-local: tests tests_with_conf
#	$(SHELL) $(testdir)/tests 2>/dev/null
#	$(SHELL) $(testdir)/tests_with_conf -- --confile $(testdir)/tests_with_conf.xml --prop-id2 -10 --ulog-no-debug 2>/dev/null
