bin_PROGRAMS		= @PACKAGE@-smemory

# не забывайте править версию в pc-файле 
USM_VER=@LIBVER@


lib_LTLIBRARIES 	= libUniSetSharedMemory.la
libUniSetSharedMemory_la_LDFLAGS	= -version-info $(USM_VER)
libUniSetSharedMemory_la_LIBADD		= $(SIGC_LIBS) $(top_builddir)/lib/libUniSet.la \
	$(top_builddir)/extensions/lib/libUniSetExtensions.la
libUniSetSharedMemory_la_CPPFLAGS	= $(SIGC_CFLAGS) -I$(top_builddir)/extensions/include
libUniSetSharedMemory_la_SOURCES 	= SharedMemory.cc 


@PACKAGE@_smemory_LDADD 	= libUniSetSharedMemory.la $(top_builddir)/lib/libUniSet.la \
	$(top_builddir)/extensions/lib/libUniSetExtensions.la
@PACKAGE@_smemory_CPPFLAGS	= $(SIGC_CFLAGS) -I$(top_builddir)/extensions/include
@PACKAGE@_smemory_SOURCES 	= smemory.cc

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSetSharedMemory.pc

all-local:
	ln -sf ../SharedMemory/$(devel_include_HEADERS) ../include
