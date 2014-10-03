############################################################################
# This file is part of the UniSet library                                  #
############################################################################

noinst_PROGRAMS 		= tests tests_with_conf
#unixml ui umutex conftest iterator_test sscanf_hex calibration threadtst dlog

tests_SOURCES = tests.cc passivetimer.cc hourglass.cc delaytimer.cc unixml.cc
tests_LDADD 	= $(top_builddir)/lib/libUniSet2.la
tests_CPPFLAGS	= -I$(top_builddir)/include -I$(includedir)/Catch

tests_with_conf_SOURCES = tests_with_conf.cc conftest.cc
tests_with_conf_LDADD 	= $(top_builddir)/lib/libUniSet2.la
tests_with_conf_CPPFLAGS	= -I$(top_builddir)/include -I$(includedir)/Catch
     
# unixml_SOURCES 	= unixml.cc
# unixml_LDADD 	= $(top_builddir)/lib/libUniSet2.la ${SIGC_LIBS}
# unixml_CPPFLAGS	= -I$(top_builddir)/include ${SIGC_CFLAGS}
# 
# iterator_test_SOURCES 	= iterator_test.cc
# iterator_test_LDADD 	= $(top_builddir)/lib/libUniSet2.la
# iterator_test_CPPFLAGS	= -I$(top_builddir)/include
# 
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
# calibration_SOURCES = calibration.cc
# calibration_LDADD 	= $(top_builddir)/lib/libUniSet2.la ${SIGC_LIBS}
# calibration_CPPFLAGS	= -I$(top_builddir)/include ${SIGC_CFLAGS}
# 
# threadtst_SOURCES = threadtst.cc
# threadtst_LDADD 	= $(top_builddir)/lib/libUniSet2.la ${SIGC_LIBS} $(COMCPP_LIBS)
# threadtst_CPPFLAGS	= -I$(top_builddir)/include ${SIGC_CFLAGS} $(COMCPP_CFLAGS)
# 
# dlog_SOURCES = dlog.cc
# dlog_LDADD 	= $(top_builddir)/lib/libUniSet2.la ${SIGC_LIBS} $(COMCPP_LIBS)
# dlog_CPPFLAGS = -I$(top_builddir)/include ${SIGC_CFLAGS} $(COMCPP_CFLAGS)
# 
