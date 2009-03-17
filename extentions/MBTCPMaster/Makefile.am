bin_PROGRAMS 			= @PACKAGE@-mbtcpmaster

lib_LTLIBRARIES 				= libUniSetMBTCPMaster.la
libUniSetMBTCPMaster_la_LIBADD		= $(top_builddir)/lib/libUniSet.la \
								$(top_builddir)/extentions/SharedMemory/libUniSetSharedMemory.la \
								$(top_builddir)/extentions/lib/libUniSetExtentions.la \
								$(SIGC_LIBS)
libUniSetMBTCPMaster_la_CXXFLAGS	= -I$(top_builddir)/extentions/include -I$(top_builddir)/extentions/SharedMemory $(SIGC_CFLAGS)
libUniSetMBTCPMaster_la_SOURCES 	= MBMaster.cc

@PACKAGE@_mbtcpmaster_SOURCES 	= main.cc
@PACKAGE@_mbtcpmaster_LDADD 	= libUniSetMBTCPMaster.la $(top_builddir)/lib/libUniSet.la \
									$(top_builddir)/extentions/SharedMemory/libUniSetSharedMemory.la \
									$(top_builddir)/extentions/lib/libUniSetExtentions.la \
									$(SIGC_LIBS)
@PACKAGE@_mbtcpmaster_CXXFLAGS	= -I$(top_builddir)/extentions/include -I$(top_builddir)/extentions/SharedMemory $(SIGC_CFLAGS)
