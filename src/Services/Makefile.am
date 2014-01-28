############################################################################
# This file is part of the UniSet library                                  #
############################################################################
noinst_LTLIBRARIES 		= libServices.la
libServices_la_CPPFLAGS	= $(SIGC_CFLAGS)
libServices_la_LIBADD 	= $(SIGC_LIBS)
libServices_la_SOURCES 	= TimerService_iSK.cc InfoServer_iSK.cc TimerService.cc DBServer.cc InfoServer.cc

include $(top_builddir)/conf/setting.mk

include $(top_builddir)/conf/setting.mk

local-clean:
	rm -rf *iSK.cc