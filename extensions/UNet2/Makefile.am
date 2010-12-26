bin_PROGRAMS 	= @PACKAGE@-unetexchange

lib_LTLIBRARIES 			= libUniSetUNet2.la
libUniSetUNet2_la_LIBADD		= $(top_builddir)/lib/libUniSet.la \
								$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
								$(top_builddir)/extensions/lib/libUniSetExtensions.la \
								$(SIGC_LIBS) $(COMCPP_LIBS)
libUniSetUNet2_la_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(COMCPP_CFLAGS)
libUniSetUNet2_la_SOURCES 	= UDPPacket.cc UNetReceiver.cc UNetSender.cc UNetExchange.cc

@PACKAGE@_unetexchange_SOURCES 		= unetexchange.cc
@PACKAGE@_unetexchange_LDADD 		= libUniSetUNet2.la $(top_builddir)/lib/libUniSet.la \
									$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSetExtensions.la \
									$(SIGC_LIBS) $(COMCPP_LIBS)
@PACKAGE@_unetexchange_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(COMCPP_CFLAGS)

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSetUNet2.pc

all-local:
	ln -sf ../UDPExchange/$(devel_include_HEADERS) ../include
