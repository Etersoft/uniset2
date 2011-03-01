bin_PROGRAMS 			= @PACKAGE@-mbtcpmaster

# не забывайте править версию в pc-файле
UMBTCP_VER=1:1:1

lib_LTLIBRARIES 				= libUniSetMBTCPMaster.la
libUniSetMBTCPMaster_la_LDFLAGS  = -version-info $(UMBTCP_VER)
libUniSetMBTCPMaster_la_LIBADD		= $(top_builddir)/lib/libUniSet.la \
								$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
								$(top_builddir)/extensions/lib/libUniSetExtensions.la \
								$(SIGC_LIBS)
libUniSetMBTCPMaster_la_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)
libUniSetMBTCPMaster_la_SOURCES 	= MBTCPMaster.cc

@PACKAGE@_mbtcpmaster_SOURCES 	= main.cc
@PACKAGE@_mbtcpmaster_LDADD 	= libUniSetMBTCPMaster.la $(top_builddir)/lib/libUniSet.la \
									$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSetExtensions.la \
									$(SIGC_LIBS)
@PACKAGE@_mbtcpmaster_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSetMBTCPMaster.pc

all-local:
	ln -sf ../MBTCPMaster/$(devel_include_HEADERS) ../include
