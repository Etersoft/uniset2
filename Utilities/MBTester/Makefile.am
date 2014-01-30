bin_PROGRAMS 	= @PACKAGE@-mbrtutest @PACKAGE@-mbrtuslave-echo \
		@PACKAGE@-mbtcpserver-echo @PACKAGE@-mbtcptest
#@PACKAGE@-mbtest

@PACKAGE@_mbrtutest_SOURCES 	= mbrtutester.cc
@PACKAGE@_mbrtutest_LDADD 	= $(top_builddir)/lib/libUniSet2.la

@PACKAGE@_mbtcptest_SOURCES 	= mbtcptester.cc
@PACKAGE@_mbtcptest_LDADD 	= $(top_builddir)/lib/libUniSet2.la $(COMCPP_LIBS)
@PACKAGE@_mbtcptest_CPPFLAGS = $(COMCPP_CFLAGS)

@PACKAGE@_mbrtuslave_echo_SOURCES 	= MBSlave.cc mbrtuslave.cc
@PACKAGE@_mbrtuslave_echo_LDADD 		= $(top_builddir)/lib/libUniSet2.la $(SIGC_LIBS)
@PACKAGE@_mbrtuslave_echo_CPPFLAGS	= $(SIGC_CFLAGS)

@PACKAGE@_mbtcpserver_echo_SOURCES 	= MBTCPServer.cc mbtcpserver.cc
@PACKAGE@_mbtcpserver_echo_LDADD 		= $(top_builddir)/lib/libUniSet2.la $(SIGC_LIBS) $(COMCPP_LIBS)
@PACKAGE@_mbtcpserver_echo_CPPFLAGS	= $(SIGC_CFLAGS) $(COMCPP_CFLAGS)

#@PACKAGE@_mbtest2_SOURCES 	= mbtester2.cc
#@PACKAGE@_mbtest2_LDADD 	= $(top_builddir)/lib/libUniSet2.la
