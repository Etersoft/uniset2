############################################################################
# This file is part of the UniSet library                                  #
############################################################################
noinst_LTLIBRARIES 	= libTCP.la
libTCP_la_SOURCES	= UTCPCore.cc UTCPStream.cc USocket.cc UTCPSocket.cc TCPCheck.cc
	
libTCP_la_CXXFLAGS	= $(SIGC_CFLAGS) $(POCO_CFLAGS)
libTCP_la_LIBADD	= $(SIGC_LIBS) $(POCO_LIBS)

include $(top_builddir)/include.mk
