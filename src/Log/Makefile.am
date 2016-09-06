############################################################################
# This file is part of the UniSet library								  #
############################################################################

noinst_LTLIBRARIES = libLog.la
libLog_la_CPPFLAGS = $(SIGC_CFLAGS) $(POCO_CFLAGS)
libLog_la_LIBADD 	=  $(SIGC_LIBS) $(POCO_LIBS)
libLog_la_SOURCES = DebugStream.cc Debug.cc LogServerTypes.cc LogServer.cc LogSession.cc LogReader.cc LogAgregator.cc

include $(top_builddir)/include.mk
