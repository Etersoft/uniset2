# не забывайте править версию в pc-файле 
ULPROC_VER=@LIBVER@

lib_LTLIBRARIES 			= libUniSetLProcessor.la
libUniSetLProcessor_la_LDFLAGS  = -version-info $(ULPROC_VER) 
libUniSetLProcessor_la_LIBADD	= $(top_builddir)/lib/libUniSet.la \
							$(top_builddir)/extensions/lib/libUniSetExtensions.la $(SIGC_LIBS)
libUniSetLProcessor_la_CXXFLAGS	= -I$(top_builddir)/extensions/include \
							-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)
libUniSetLProcessor_la_SOURCES 	= Element.cc TOR.cc TAND.cc TDelay.cc TNOT.cc \
Schema.cc SchemaXML.cc LProcessor.cc PassiveLProcessor.cc


bin_PROGRAMS 		= @PACKAGE@-logicproc @PACKAGE@-plogicproc

@PACKAGE@_logicproc_SOURCES 	= logicproc.cc
@PACKAGE@_logicproc_LDADD 	= libUniSetLProcessor.la \
						$(top_builddir)/extensions/lib/libUniSetExtensions.la \
						$(top_builddir)/lib/libUniSet.la $(SIGC_LIBS)
@PACKAGE@_logicproc_CXXFLAGS	= -I$(top_builddir)/extensions/include \
				-I$(top_builddir)/extensions/SharedMemory \
				$(SIGC_CFLAGS)

@PACKAGE@_plogicproc_SOURCES 	= plogicproc.cc
@PACKAGE@_plogicproc_LDADD 	= libUniSetLProcessor.la \
						$(top_builddir)/extensions/lib/libUniSetExtensions.la \
						$(top_builddir)/lib/libUniSet.la $(SIGC_LIBS)
@PACKAGE@_plogicproc_CXXFLAGS	= -I$(top_builddir)/extensions/include \
				-I$(top_builddir)/extensions/SharedMemory \
				$(SIGC_CFLAGS)

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSetLogicProcessor.pc

all-local:
	ln -sf ../LogicProcessor/$(devel_include_HEADERS) ../include
