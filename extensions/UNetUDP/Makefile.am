bin_PROGRAMS = @PACKAGE@-unetexchange @PACKAGE@-unet-udp-tester

# не забывайте править версию в pc-файле
UNETUDP_VER=@LIBVER@

lib_LTLIBRARIES 			= libUniSetUNetUDP.la
libUniSetUNetUDP_la_LDFLAGS  = -version-info $(UNETUDP_VER)
libUniSetUNetUDP_la_LIBADD		= $(top_builddir)/lib/libUniSet.la \
								$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
								$(top_builddir)/extensions/lib/libUniSetExtensions.la \
								$(SIGC_LIBS) $(COMCPP_LIBS)
libUniSetUNetUDP_la_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(COMCPP_CFLAGS)
libUniSetUNetUDP_la_SOURCES 	= UDPPacket.cc UNetReceiver.cc UNetSender.cc UNetExchange.cc

@PACKAGE@_unetexchange_SOURCES 		= unetexchange.cc
@PACKAGE@_unetexchange_LDADD 		= libUniSetUNetUDP.la $(top_builddir)/lib/libUniSet.la \
									$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSetExtensions.la \
									$(SIGC_LIBS) $(COMCPP_LIBS)
@PACKAGE@_unetexchange_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(COMCPP_CFLAGS)


@PACKAGE@_unet_udp_tester_SOURCES	= UDPPacket.cc unet-udp-tester.cc
@PACKAGE@_unet_udp_tester_LDADD 	= $(COMCPP_LIBS)
@PACKAGE@_unet_udp_tester_CXXFLAGS	= $(COMCPP_CFLAGS)

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSetUNetUDP.pc

all-local:
	ln -sf ../UNetUDP/$(devel_include_HEADERS) ../include
