############################################################################
# This file is part of the UniSet library                                  #
############################################################################

#check_PROGRAMS = tests tests_with_conf
noinst_PROGRAMS = tests tests_with_conf

#umutex threadtst dlog
tests_SOURCES = tests.cc passivetimer.cc hourglass.cc delaytimer.cc unixml.cc sscanf_hex.cc \
	callbacktimer.cc trigger.cc triggerOR.cc triggerAND.cc triggerOUT.cc pulse.cc \
	modbustypes.cc
tests_LDADD 	= $(top_builddir)/lib/libUniSet2.la $(SIGC_LIBS) $(COMCPP_LIBS)
tests_CPPFLAGS	= -I$(top_builddir)/include $(SIGC_CFLAGS) $(COMCPP_CFLAGS)

tests_with_conf_SOURCES = tests_with_conf.cc conftest.cc ui.cc
tests_with_conf_LDADD   = $(top_builddir)/lib/libUniSet2.la
tests_with_conf_CPPFLAGS = -I$(top_builddir)/include
     
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
