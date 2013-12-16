############################################################################
# This file is part of the UniSet library                                  #
############################################################################

noinst_LTLIBRARIES = libVarious.la
libVarious_la_CPPFLAGS 	= $(SIGC_CFLAGS) $(COMCPP_CFLAGS)
libVarious_la_LIBADD 	= $(SIGC_LIBS) $(COMCPP_LIBS)
libVarious_la_SOURCES 	= DebugStream.cc Debug.cc UniXML.cc MessageType.cc Configuration.cc \
	Restorer_XML.cc RunLock.cc Mutex.cc SViewer.cc SMonitor.cc LT_Object.cc WDTInterface.cc

include $(top_builddir)/conf/setting.mk

local-clean:
	rm -rf *iSK.cc
	