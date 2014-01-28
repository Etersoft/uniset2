bin_PROGRAMS		= @PACKAGE@-network

# не забывайте править версию в pc-файле 
UNET_VER=@LIBVER@

lib_LTLIBRARIES 	= libUniSetNetwork.la
libUniSetNetwork_la_LDFLAGS  = -version-info $(UNET_VER) 
libUniSetNetwork_la_LIBADD = $(SIGC_LIBS) $(top_builddir)/lib/libUniSet.la \
	$(top_builddir)/extensions/lib/libUniSetExtensions.la \
	$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la
libUniSetNetwork_la_CPPFLAGS	= $(SIGC_CFLAGS) \
	-I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory
libUniSetNetwork_la_SOURCES 	= UniExchange.cc 

@PACKAGE@_network_LDADD 	= libUniSetNetwork.la \
	$(top_builddir)/lib/libUniSet.la \
	$(top_builddir)/extensions/lib/libUniSetExtensions.la
@PACKAGE@_network_CPPFLAGS	= $(SIGC_CFLAGS) -I$(top_builddir)/extensions/include
@PACKAGE@_network_SOURCES 	= uninet.cc

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSetNetwork.pc

all-local:
	ln -sf ../UniNetwork/$(devel_include_HEADERS) ../include
