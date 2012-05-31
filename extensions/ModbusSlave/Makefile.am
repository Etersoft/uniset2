bin_PROGRAMS = @PACKAGE@-mbslave

# не забывайте править версию в pc-файле
UMBS_VER=@LIBVER@

lib_LTLIBRARIES 				= libUniSetMBSlave.la
libUniSetMBSlave_la_LDFLAGS  = -version-info $(UMBS_VER)
libUniSetMBSlave_la_LIBADD		= $(top_builddir)/lib/libUniSet.la \
								$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
								$(top_builddir)/extensions/lib/libUniSetExtensions.la \
								$(SIGC_LIBS)
libUniSetMBSlave_la_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)
libUniSetMBSlave_la_SOURCES 	= MBSlave.cc

@PACKAGE@_mbslave_SOURCES 	= mbslave.cc
@PACKAGE@_mbslave_LDADD 	= libUniSetMBSlave.la $(top_builddir)/lib/libUniSet.la \
									$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSetExtensions.la \
									$(SIGC_LIBS)
@PACKAGE@_mbslave_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSetMBSlave.pc

all-local:
	ln -sf ../ModbusSlave/$(devel_include_HEADERS) ../include
