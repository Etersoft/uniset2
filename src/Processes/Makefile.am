############################################################################
# This file is part of the UniSet library								  #
############################################################################

noinst_LTLIBRARIES 			= libProcesses.la
libProcesses_la_CXXFLAGS 	= $(SIGC_CFLAGS)
libProcesses_la_LIBADD		= $(SIGC_LIBS)
libProcesses_la_SOURCES		= IOController_iSK.cc IOController.cc IONotifyController.cc \
	NCRestorer.cc NCRestorer_XML.cc EventLoopServer.cc

local-clean:
	rm -rf *iSK.cc


include $(top_builddir)/include.mk
