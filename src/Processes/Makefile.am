############################################################################
# This file is part of the UniSet library                                  #
############################################################################

noinst_LTLIBRARIES 			= libProcesses.la
libProcesses_la_CXXFLAGS 	= $(SIGC_CFLAGS)
libProcesses_la_LIBADD		= $(SIGC_LIBS)
libProcesses_la_SOURCES		= IOController_iSK.cc IOController.cc IONotifyController.cc IONotifyController_LT.cc \
	NCRestorer.cc NCRestorer_XML.cc

include $(top_builddir)/conf/setting.mk

local-clean:
	rm -rf *iSK.cc