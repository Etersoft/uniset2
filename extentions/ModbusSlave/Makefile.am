bin_PROGRAMS 			= @PACKAGE@-mbslave

lib_LTLIBRARIES 				= libUniSetMBSlave.la
libUniSetMBSlave_la_LIBADD		= $(top_builddir)/lib/libUniSet.la \
								$(top_builddir)/extentions/SharedMemory/libUniSetSharedMemory.la \
								$(top_builddir)/extentions/lib/libUniSetExtentions.la \
								$(SIGC_LIBS)
libUniSetMBSlave_la_CXXFLAGS	= -I$(top_builddir)/extentions/include -I$(top_builddir)/extentions/SharedMemory $(SIGC_CFLAGS)
libUniSetMBSlave_la_SOURCES 	= MBSlave.cc

@PACKAGE@_mbslave_SOURCES 	= mbslave.cc
@PACKAGE@_mbslave_LDADD 	= libUniSetMBSlave.la $(top_builddir)/lib/libUniSet.la \
									$(top_builddir)/extentions/SharedMemory/libUniSetSharedMemory.la \
									$(top_builddir)/extentions/lib/libUniSetExtentions.la \
									$(SIGC_LIBS)
@PACKAGE@_mbslave_CXXFLAGS	= -I$(top_builddir)/extentions/include -I$(top_builddir)/extentions/SharedMemory $(SIGC_CFLAGS)

# install
devel_include_HEADERS = *.h
devel_includedir = $(includedir)/@PACKAGE@/extentions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSetMBSlave.pc
