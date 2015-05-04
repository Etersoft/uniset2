if DISABLE_LOGICPROC

else

# не забывайте править версию в2.pc-файле
ULPROC_VER=@LIBVER@

lib_LTLIBRARIES = libUniSet2LProcessor.la
libUniSet2LProcessor_la_LDFLAGS  = -version-info $(ULPROC_VER)
libUniSet2LProcessor_la_LIBADD	= $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la $(SIGC_LIBS)
libUniSet2LProcessor_la_CXXFLAGS	= -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)
libUniSet2LProcessor_la_SOURCES 	= Element.cc TOR.cc TAND.cc TDelay.cc TNOT.cc \
Schema.cc SchemaXML.cc LProcessor.cc PassiveLProcessor.cc

bin_PROGRAMS = @PACKAGE@-logicproc @PACKAGE@-plogicproc

@PACKAGE@_logicproc_SOURCES = logicproc.cc
@PACKAGE@_logicproc_LDADD   = libUniSet2LProcessor.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/lib/libUniSet2.la $(SIGC_LIBS)
@PACKAGE@_logicproc_CXXFLAGS	= -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory \
	$(SIGC_CFLAGS)

@PACKAGE@_plogicproc_SOURCES 	= plogicproc.cc
@PACKAGE@_plogicproc_LDADD 	= libUniSet2LProcessor.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/lib/libUniSet2.la $(SIGC_LIBS)
@PACKAGE@_plogicproc_CXXFLAGS	= -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory \
	$(SIGC_CFLAGS)

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet2LogicProcessor.pc

all-local:
	ln -sf ../LogicProcessor/$(devel_include_HEADERS) ../include

# SUBDIRS=tests

include $(top_builddir)/include.mk

endif
