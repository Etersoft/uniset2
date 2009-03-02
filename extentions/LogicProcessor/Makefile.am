lib_LTLIBRARIES 			= libUniSetLProcessor.la
libUniSetLProcessor_la_LIBADD		= $(top_builddir)/lib/libUniSet.la \
							$(top_builddir)/extentions/lib/libUniSetExtentions.la $(SIGC_LIBS)
libUniSetLProcessor_la_CXXFLAGS	= -I$(top_builddir)/extentions/include \
							-I$(top_builddir)/extentions/SharedMemory \
							$(SIGC_CFLAGS)
libUniSetLProcessor_la_SOURCES 	= Element.cc TOR.cc TAND.cc TDelay.cc TNOT.cc \
Schema.cc SchemaXML.cc LProcessor.cc PassiveLProcessor.cc


bin_PROGRAMS 		= @PACKAGE@-logicproc @PACKAGE@-plogicproc

@PACKAGE@_logicproc_SOURCES 	= logicproc.cc
@PACKAGE@_logicproc_LDADD 	= libUniSetLProcessor.la \
						$(top_builddir)/extentions/lib/libUniSetExtentions.la \
						$(top_builddir)/lib/libUniSet.la $(SIGC_LIBS)
@PACKAGE@_logicproc_CXXFLAGS	= -I$(top_builddir)/extentions/include \
				-I$(top_builddir)/extentions/SharedMemory \
				$(SIGC_CFLAGS)

@PACKAGE@_plogicproc_SOURCES 	= plogicproc.cc
@PACKAGE@_plogicproc_LDADD 	= libUniSetLProcessor.la \
						$(top_builddir)/extentions/lib/libUniSetExtentions.la \
						$(top_builddir)/lib/libUniSet.la $(SIGC_LIBS)
@PACKAGE@_plogicproc_CXXFLAGS	= -I$(top_builddir)/extentions/include \
				-I$(top_builddir)/extentions/SharedMemory \
				$(SIGC_CFLAGS)

# install
devel_include_HEADERS = *.h
devel_includedir = $(includedir)/@PACKAGE@/extentions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSetLogicProcessor.pc
