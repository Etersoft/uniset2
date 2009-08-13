############################################################################
# This file is part of the UniSet library                                  #
############################################################################

noinst_PROGRAMS 		= passivetimer unixml


passivetimer_SOURCES 	= passivetimer.cc
passivetimer_LDADD 		= $(top_builddir)/lib/libUniSet.la
passivetimer_CPPFLAGS	= -I$(top_builddir)/include

unixml_SOURCES 	= unixml.cc
unixml_LDADD 	= $(top_builddir)/lib/libUniSet.la
unixml_CPPFLAGS	= -I$(top_builddir)/include

include $(top_builddir)/conf/setting.mk

