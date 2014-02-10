############################################################################
# This file is part of the UniSet library                                  #
############################################################################

noinst_PROGRAMS 		= passivetimer hourglass delaytimer unixml ui umutex conftest iterator_test sscanf_hex calibration threadtst dlog

passivetimer_SOURCES 	= passivetimer.cc
passivetimer_LDADD 		= $(top_builddir)/lib/libUniSet2.la
passivetimer_CPPFLAGS	= -I$(top_builddir)/include

hourglass_SOURCES 	= hourglass.cc
hourglass_LDADD 	= $(top_builddir)/lib/libUniSet2.la
hourglass_CPPFLAGS	= -I$(top_builddir)/include

delaytimer_SOURCES 	= delaytimer.cc
delaytimer_LDADD 	= $(top_builddir)/lib/libUniSet2.la
delaytimer_CPPFLAGS	= -I$(top_builddir)/include

unixml_SOURCES 	= unixml.cc
unixml_LDADD 	= $(top_builddir)/lib/libUniSet2.la ${SIGC_LIBS}
unixml_CPPFLAGS	= -I$(top_builddir)/include ${SIGC_CFLAGS}

iterator_test_SOURCES 	= iterator_test.cc
iterator_test_LDADD 	= $(top_builddir)/lib/libUniSet2.la
iterator_test_CPPFLAGS	= -I$(top_builddir)/include

ui_SOURCES 	= ui.cc
ui_LDADD 	= $(top_builddir)/lib/libUniSet2.la
ui_CPPFLAGS	= -I$(top_builddir)/include

umutex_SOURCES 	= umutex.cc
umutex_LDADD 	= $(top_builddir)/lib/libUniSet2.la $(COMCPP_LIBS)
umutex_CPPFLAGS	= -I$(top_builddir)/include $(COMCPP_CFLAGS)

conftest_SOURCES 	= conftest.cc
conftest_LDADD 	= $(top_builddir)/lib/libUniSet2.la
conftest_CPPFLAGS	= -I$(top_builddir)/include

sscanf_hex_SOURCES 	= sscanf_hex.cc

calibration_SOURCES = calibration.cc
calibration_LDADD 	= $(top_builddir)/lib/libUniSet2.la ${SIGC_LIBS}
calibration_CPPFLAGS	= -I$(top_builddir)/include ${SIGC_CFLAGS}

threadtst_SOURCES = threadtst.cc
threadtst_LDADD 	= $(top_builddir)/lib/libUniSet2.la ${SIGC_LIBS} $(COMCPP_LIBS)
threadtst_CPPFLAGS	= -I$(top_builddir)/include ${SIGC_CFLAGS} $(COMCPP_CFLAGS)

dlog_SOURCES = dlog.cc
dlog_LDADD 	= $(top_builddir)/lib/libUniSet2.la ${SIGC_LIBS} $(COMCPP_LIBS)
dlog_CPPFLAGS = -I$(top_builddir)/include ${SIGC_CFLAGS} $(COMCPP_CFLAGS)


include $(top_builddir)/conf/setting.mk

