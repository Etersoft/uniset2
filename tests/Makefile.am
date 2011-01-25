############################################################################
# This file is part of the UniSet library                                  #
############################################################################

SUBDIRS=JrnTests

noinst_PROGRAMS 		= passivetimer unixml ui umutex conftest

passivetimer_SOURCES 	= passivetimer.cc
passivetimer_LDADD 		= $(top_builddir)/lib/libUniSet.la
passivetimer_CPPFLAGS	= -I$(top_builddir)/include

unixml_SOURCES 	= unixml.cc
unixml_LDADD 	= $(top_builddir)/lib/libUniSet.la ${SIGC_LIBS}
unixml_CPPFLAGS	= -I$(top_builddir)/include ${SIGC_CFLAGS}

ui_SOURCES 	= ui.cc
ui_LDADD 	= $(top_builddir)/lib/libUniSet.la
ui_CPPFLAGS	= -I$(top_builddir)/include

umutex_SOURCES 	= umutex.cc
umutex_LDADD 	= $(top_builddir)/lib/libUniSet.la
umutex_CPPFLAGS	= -I$(top_builddir)/include

conftest_SOURCES 	= conftest.cc
conftest_LDADD 	= $(top_builddir)/lib/libUniSet.la
conftest_CPPFLAGS	= -I$(top_builddir)/include


include $(top_builddir)/conf/setting.mk

