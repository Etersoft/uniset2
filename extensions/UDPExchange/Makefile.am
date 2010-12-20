bin_PROGRAMS 	= @PACKAGE@-udpexchange
noinst_PROGRAMS = @PACKAGE@-udpreceiver @PACKAGE@-unetexchange
#@PACKAGE@-udpsender

UUDP_VER=@LIBVER@

lib_LTLIBRARIES 			= libUniSetUDP.la
libUniSetUDP_la_LDFLAGS  	= -version-info $(UUDP_VER)
libUniSetUDP_la_LIBADD		= $(top_builddir)/lib/libUniSet.la \
								$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
								$(top_builddir)/extensions/lib/libUniSetExtensions.la \
								$(SIGC_LIBS) $(COMCPP_LIBS)
libUniSetUDP_la_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(COMCPP_CFLAGS)
libUniSetUDP_la_SOURCES 	= UDPPacket.cc UDPExchange.cc UDPNReceiver.cc UDPReceiver.cc UNetReceiver.cc UNetExchange.cc

#UDPSender.cc
#UDPSender.cc

@PACKAGE@_udpexchange_SOURCES 	= udpexchange.cc
@PACKAGE@_udpexchange_LDADD 	= libUniSetUDP.la $(top_builddir)/lib/libUniSet.la \
									$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSetExtensions.la \
									$(SIGC_LIBS) $(COMCPP_LIBS)
@PACKAGE@_udpexchange_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(COMCPP_CFLAGS)

@PACKAGE@_udpsender_SOURCES 	= udpsender.cc
@PACKAGE@_udpsender_LDADD 		= libUniSetUDP.la $(top_builddir)/lib/libUniSet.la \
									$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSetExtensions.la \
									$(SIGC_LIBS) $(COMCPP_LIBS)
@PACKAGE@_udpsender_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(COMCPP_CFLAGS)

@PACKAGE@_udpreceiver_SOURCES 		= udpreceiver.cc
@PACKAGE@_udpreceiver_LDADD 		= libUniSetUDP.la $(top_builddir)/lib/libUniSet.la \
									$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSetExtensions.la \
									$(SIGC_LIBS) $(COMCPP_LIBS)
@PACKAGE@_udpreceiver_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(COMCPP_CFLAGS)

@PACKAGE@_unetexchange_SOURCES 		= unetexchange.cc
@PACKAGE@_unetexchange_LDADD 		= libUniSetUDP.la $(top_builddir)/lib/libUniSet.la \
									$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSetExtensions.la \
									$(SIGC_LIBS) $(COMCPP_LIBS)
@PACKAGE@_unetexchange_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(COMCPP_CFLAGS)


# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSetUDP.pc

all-local:
	ln -sf ../UDPExchange/$(devel_include_HEADERS) ../include
