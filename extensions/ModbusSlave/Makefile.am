bin_PROGRAMS = @PACKAGE@-mbslave

# не забывайте править версию в2.pc-файле
UMBS_VER=@LIBVER@

lib_LTLIBRARIES 			  = libUniSet2MBSlave.la
libUniSet2MBSlave_la_LDFLAGS  = -version-info $(UMBS_VER)
libUniSet2MBSlave_la_LIBADD		= $(top_builddir)/lib/libUniSet2.la \
								$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
								$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
								$(SIGC_LIBS) $(POCO_LIBS)
libUniSet2MBSlave_la_CXXFLAGS	= -I$(top_srcdir)/extensions/include -I$(top_srcdir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS)
libUniSet2MBSlave_la_SOURCES 	= MBSlave.cc

@PACKAGE@_mbslave_SOURCES 	= mbslave.cc
@PACKAGE@_mbslave_LDADD 	= libUniSet2MBSlave.la $(top_builddir)/lib/libUniSet2.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
									$(SIGC_LIBS) $(POCO_LIBS)
@PACKAGE@_mbslave_CXXFLAGS	= -I$(top_srcdir)/extensions/include -I$(top_srcdir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS)

# install
devel_include_HEADERS = $(wildcard $(srcdir)/*.h)
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet2MBSlave.pc

#all-local:
#	ln -sf ../ModbusSlave/$(devel_include_HEADERS) ../include

include $(top_builddir)/include.mk

# SUBDIRS=tests
