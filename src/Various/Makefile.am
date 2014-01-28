############################################################################
# This file is part of the UniSet library                                  #
############################################################################

noinst_LTLIBRARIES = libVarious.la
libVarious_la_CPPFLAGS 	= $(SIGC_CFLAGS)
libVarious_la_LIBADD 	= $(SIGC_LIBS)
libVarious_la_SOURCES 	= DebugStream.cc Debug.cc UniXML.cc MessageType.cc Configuration.cc SystemGuard.cc TextIndex.cc \
	Restorer_XML.cc \
	NCRestorer.cc NCRestorer_XML.cc \
	ISRestorer.cc ISRestorer_XML.cc \
	RunLock.cc Mutex.cc SViewer.cc SMonitor.cc LT_Object.cc \
	MessageInterface_XML.cc MessageInterface_idXML.cc WDTInterface.cc \
	CycleStorage.cc TableStorage.cc TableBlockStorage.cc

include $(top_builddir)/conf/setting.mk

local-clean:
	rm -rf *iSK.cc
	