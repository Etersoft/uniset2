############################################################################
# This file is part of the UniSet library                                  #
############################################################################

noinst_PROGRAMS 		= passivetimer ui

passivetimer_SOURCES 	= passivetimer.cc
passivetimer_LDADD 		= $(top_builddir)/lib/libUniSet.la
passivetimer_CPPFLAGS	= -I$(top_builddir)/include

ui_SOURCES 	= ui.cc
ui_LDADD 	= $(top_builddir)/lib/libUniSet.la
ui_CPPFLAGS	= -I$(top_builddir)/include


include $(top_builddir)/conf/setting.mk

