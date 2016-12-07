############################################################################
# This file is part of the UniSet library								  #
############################################################################

noinst_LTLIBRARIES = libVarious.la
libVarious_la_CPPFLAGS 	= $(SIGC_CFLAGS) $(POCO_CFLAGS)
libVarious_la_LIBADD 	= $(SIGC_LIBS) $(POCO_LIBS)
libVarious_la_SOURCES 	= UniXML.cc MessageType.cc Configuration.cc MQMutex.cc MQAtomic.cc \
	Restorer_XML.cc RunLock.cc Mutex.cc SViewer.cc SMonitor.cc LT_Object.cc WDTInterface.cc VMonitor.cc \
	ujson.cc

local-clean:
	rm -rf *iSK.cc
	

include $(top_builddir)/include.mk
