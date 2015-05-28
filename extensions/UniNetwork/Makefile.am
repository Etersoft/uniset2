bin_PROGRAMS		= @PACKAGE@-network

# не забывайте править версию в2.pc-файле
UNET_VER=@LIBVER@

lib_LTLIBRARIES 	= libUniSet2Network.la
libUniSet2Network_la_LDFLAGS  = -version-info $(UNET_VER)
libUniSet2Network_la_LIBADD = $(SIGC_LIBS) $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(COMCPP_LIBS)
libUniSet2Network_la_CPPFLAGS	= $(SIGC_CFLAGS) $(COMCPP_CFLAGS)\
	-I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory
libUniSet2Network_la_SOURCES = UniExchange.cc

@PACKAGE@_network_LDADD 	= libUniSet2Network.la \
	$(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(COMCPP_LIBS)
@PACKAGE@_network_CPPFLAGS	= $(SIGC_CFLAGS) $(COMCPP_CFLAGS) -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory
@PACKAGE@_network_SOURCES 	= uninet.cc

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet2Network.pc

#all-local:
#	ln -sf ../UniNetwork/$(devel_include_HEADERS) ../include

include $(top_builddir)/include.mk
