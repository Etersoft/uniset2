bin_PROGRAMS 			= @PACKAGE@-rtuexchange mtrconv rtustate

lib_LTLIBRARIES 			= libUniSetRTU.la
libUniSetRTU_la_LIBADD		= $(top_builddir)/lib/libUniSet.la \
								$(top_builddir)/extentions/SharedMemory/libUniSetSharedMemory.la \
								$(top_builddir)/extentions/lib/libUniSetExtentions.la \
								$(SIGC_LIBS)
libUniSetRTU_la_CXXFLAGS	= -I$(top_builddir)/extentions/include -I$(top_builddir)/extentions/SharedMemory $(SIGC_CFLAGS)
libUniSetRTU_la_SOURCES 	= RTUStorage.cc RTUExchange.cc

@PACKAGE@_rtuexchange_SOURCES 	= rtuexchange.cc
@PACKAGE@_rtuexchange_LDADD 	= libUniSetRTU.la $(top_builddir)/lib/libUniSet.la \
									$(top_builddir)/extentions/SharedMemory/libUniSetSharedMemory.la \
									$(top_builddir)/extentions/lib/libUniSetExtentions.la \
									$(SIGC_LIBS)
@PACKAGE@_rtuexchange_CXXFLAGS	= -I$(top_builddir)/extentions/include -I$(top_builddir)/extentions/SharedMemory $(SIGC_CFLAGS)

mtrconv_SOURCES	= mtrconv.cc
mtrconv_LDADD	= $(top_builddir)/extentions/lib/libUniSetExtentions.la $(top_builddir)/lib/libUniSet.la
mtrconv_CXXFLAGS = -I$(top_builddir)/extentions/include

rtustate_LDADD 		= libUniSetRTU.la $(top_builddir)/extentions/lib/libUniSetExtentions.la $(top_builddir)/lib/libUniSet.la
rtustate__CXXFLAGS	= -I$(top_builddir)/extentions/include
rtustate_SOURCES	= rtustate.cc

# install
devel_include_HEADERS = *.h
devel_includedir = $(includedir)/@PACKAGE@/extentions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSetRTU.pc
