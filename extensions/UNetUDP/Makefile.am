bin_PROGRAMS = @PACKAGE@-unetexchange @PACKAGE@-unet-udp-tester

# не забывайте править версию в2.pc-файле
UNETUDP_VER=@LIBVER@

lib_LTLIBRARIES 			= libUniSet2UNetUDP.la
libUniSet2UNetUDP_la_LDFLAGS  = -version-info $(UNETUDP_VER)
libUniSet2UNetUDP_la_LIBADD		= $(top_builddir)/lib/libUniSet2.la \
								$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
								$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
								$(SIGC_LIBS) $(POCO_LIBS)
libUniSet2UNetUDP_la_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS)
libUniSet2UNetUDP_la_SOURCES 	= UDPPacket.cc UDPTransport.cc MulticastTransport.cc UNetReceiver.cc UNetSender.cc UNetExchange.cc

@PACKAGE@_unetexchange_SOURCES 		= unetexchange.cc
@PACKAGE@_unetexchange_LDADD 		= libUniSet2UNetUDP.la $(top_builddir)/lib/libUniSet2.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
									$(SIGC_LIBS) $(POCO_LIBS)
@PACKAGE@_unetexchange_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS)


@PACKAGE@_unet_udp_tester_SOURCES	= UDPPacket.cc unet-udp-tester.cc
@PACKAGE@_unet_udp_tester_LDADD 	= $(top_builddir)/lib/libUniSet2.la $(POCO_LIBS)
@PACKAGE@_unet_udp_tester_CXXFLAGS	= $(POCO_CFLAGS)

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet2UNetUDP.pc

#all-local:
#	ln -sf ../UNetUDP/$(devel_include_HEADERS) ../include

include $(top_builddir)/include.mk
