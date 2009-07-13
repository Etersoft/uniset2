bin_PROGRAMS		= @PACKAGE@-smemory

lib_LTLIBRARIES 	= libUniSetSharedMemory.la
libUniSetSharedMemory_la_LIBADD		= $(SIGC_LIBS) $(top_builddir)/lib/libUniSet.la \
	$(top_builddir)/extensions/lib/libUniSetExtensions.la
libUniSetSharedMemory_la_CPPFLAGS	= $(SIGC_CFLAGS) -I$(top_builddir)/extensions/include
libUniSetSharedMemory_la_SOURCES 	= SharedMemory.cc 

@PACKAGE@_smemory_LDADD 	= libUniSetSharedMemory.la
@PACKAGE@_smemory_CPPFLAGS	= $(SIGC_CFLAGS) -I$(top_builddir)/extensions/include
@PACKAGE@_smemory_SOURCES 	= smemory.cc

include $(top_builddir)/conf/setting.mk

# install
devel_include_HEADERS = *.h
devel_includedir = $(includedir)/@PACKAGE@/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSetSharedMemory.pc
