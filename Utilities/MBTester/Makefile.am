bin_PROGRAMS 	= uniset-mbrtutest uniset-mbrtuslave-echo \
		uniset-mbtcpserver-echo uniset-mbtcptest
#uniset-mbtest

uniset_mbrtutest_SOURCES 	= mbrtutester.cc
uniset_mbrtutest_LDADD 	= $(top_builddir)/lib/libUniSet.la

uniset_mbtcptest_SOURCES 	= mbtcptester.cc
uniset_mbtcptest_LDADD 	= $(top_builddir)/lib/libUniSet.la $(COMCPP_LIBS)
uniset_mbtcptest_CPPFLAGS = $(COMCPP_CFLAGS)

uniset_mbrtuslave_echo_SOURCES 	= MBSlave.cc mbrtuslave.cc
uniset_mbrtuslave_echo_LDADD 		= $(top_builddir)/lib/libUniSet.la $(SIGC_LIBS)
uniset_mbrtuslave_echo_CPPFLAGS	= $(SIGC_CFLAGS)

uniset_mbtcpserver_echo_SOURCES 	= MBTCPServer.cc mbtcpserver.cc
uniset_mbtcpserver_echo_LDADD 		= $(top_builddir)/lib/libUniSet.la $(SIGC_LIBS) $(COMCPP_LIBS)
uniset_mbtcpserver_echo_CPPFLAGS	= $(SIGC_CFLAGS) $(COMCPP_CFLAGS)

#uniset_mbtest2_SOURCES 	= mbtester2.cc
#uniset_mbtest2_LDADD 	= $(top_builddir)/lib/libUniSet.la
