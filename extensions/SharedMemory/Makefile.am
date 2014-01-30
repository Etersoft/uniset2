bin_PROGRAMS		= @PACKAGE@-smemory

# не забывайте править версию в2.pc-файле
USM_VER=@LIBVER@


lib_LTLIBRARIES 	= libUniSet2SharedMemory.la
libUniSet2SharedMemory.la_LDFLAGS	= -version-info $(USM_VER)
libUniSet2SharedMemory.la_LIBADD		= $(SIGC_LIBS) $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la
libUniSet2SharedMemory.la_CPPFLAGS	= $(SIGC_CFLAGS) -I$(top_builddir)/extensions/include
libUniSet2SharedMemory.la_SOURCES 	= SharedMemory.cc


@PACKAGE@_smemory_LDADD 	= libUniSet2SharedMemory.la $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la
@PACKAGE@_smemory_CPPFLAGS	= $(SIGC_CFLAGS) -I$(top_builddir)/extensions/include
@PACKAGE@_smemory_SOURCES 	= smemory.cc

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet2SharedMemory.pc

all-local:
	ln -sf ../SharedMemory/$(devel_include_HEADERS) ../include
