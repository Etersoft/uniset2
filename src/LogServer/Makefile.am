############################################################################
# This file is part of the UniSet library                                  #
############################################################################

noinst_LTLIBRARIES = libLogServer.la
libLogServer_la_CPPFLAGS = $(SIGC_CFLAGS) $(COMCPP_CFLAGS)
#libLogServer_la_LIBADD 	=  $(top_builddir)/src/Communications/TCP/libTCP.la $(SIGC_LIBS) $(COMCPP_LIBS)
libLogServer_la_LIBADD 	=  $(SIGC_LIBS) $(COMCPP_LIBS)
libLogServer_la_SOURCES = LogServer.cc LogSession.cc LogReader.cc
