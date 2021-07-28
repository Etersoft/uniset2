############################################################################
# This file is part of the UniSet library								  #
############################################################################

noinst_LTLIBRARIES 			= libProcesses.la
libProcesses_la_CXXFLAGS 	= -I$(top_builddir)/include $(SIGC_CFLAGS) $(EV_CFLAGS)
libProcesses_la_LIBADD		= $(SIGC_LIBS) $(EV_LIBS)
libProcesses_la_SOURCES		= IOController_iSK.cc IOController.cc IONotifyController.cc \
	IOConfig_XML.cc EventLoopServer.cc CommonEventLoop.cc ProxyManager.cc PassiveObject.cc \
	RunLock.cc

# NCRestorer.cc NCRestorer_XML.cc

local-clean:
	rm -rf *iSK.cc


include $(top_builddir)/include.mk
