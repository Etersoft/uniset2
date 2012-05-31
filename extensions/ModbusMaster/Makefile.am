noinst_LTLIBRARIES 		= libMBMaster.la
libMBMaster_la_LIBADD	= $(top_builddir)/lib/libUniSet.la \
								$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
								$(top_builddir)/extensions/lib/libUniSetExtensions.la \
								$(SIGC_LIBS)
libMBMaster_la_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)
libMBMaster_la_SOURCES 	= MBExchange.cc RTUStorage.cc

bin_PROGRAMS 		= @PACKAGE@-mbtcpmaster @PACKAGE@-rtuexchange @PACKAGE@-mtr-conv @PACKAGE@-rtu188-state @PACKAGE@-vtconv @PACKAGE@-mtr-setup @PACKAGE@-mtr-read
lib_LTLIBRARIES 	= libUniSetMBTCPMaster.la libUniSetRTU.la

# -------------------- Mobus TCP -------------------
# не забывайте править версию в pc-файле
UMBTCP_VER=@LIBVER@


libUniSetMBTCPMaster_la_LDFLAGS  = -version-info $(UMBTCP_VER)
libUniSetMBTCPMaster_la_LIBADD	 = libMBMaster.la $(top_builddir)/lib/libUniSet.la \
								$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
								$(top_builddir)/extensions/lib/libUniSetExtensions.la \
								$(SIGC_LIBS)
libUniSetMBTCPMaster_la_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)
libUniSetMBTCPMaster_la_SOURCES 	= MBTCPMaster.cc

@PACKAGE@_mbtcpmaster_SOURCES 	= main.cc
@PACKAGE@_mbtcpmaster_LDADD 	= libUniSetMBTCPMaster.la libMBMaster.la $(top_builddir)/lib/libUniSet.la \
									$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSetExtensions.la \
									$(SIGC_LIBS)
@PACKAGE@_mbtcpmaster_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)

# -------------------- Mobus RTU -------------------
# не забывайте править версию в pc-файле
URTU_VER=2:1:0

libUniSetRTU_la_LDFLAGS  = -version-info $(URTU_VER)
libUniSetRTU_la_LIBADD		= libMBMaster.la $(top_builddir)/lib/libUniSet.la \
								$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
								$(top_builddir)/extensions/lib/libUniSetExtensions.la \
								$(SIGC_LIBS)
libUniSetRTU_la_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)
libUniSetRTU_la_SOURCES 	= RTUExchange.cc

@PACKAGE@_rtuexchange_SOURCES 	= rtuexchange.cc
@PACKAGE@_rtuexchange_LDADD 	= libUniSetRTU.la libMBMaster.la $(top_builddir)/lib/libUniSet.la \
									$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSetExtensions.la \
									$(SIGC_LIBS)
@PACKAGE@_rtuexchange_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)

@PACKAGE@_mtr_conv_SOURCES	= mtr-conv.cc
@PACKAGE@_mtr_conv_LDADD	= $(top_builddir)/extensions/lib/libUniSetExtensions.la $(top_builddir)/lib/libUniSet.la
@PACKAGE@_mtr_conv_CXXFLAGS = -I$(top_builddir)/extensions/include

@PACKAGE@_mtr_setup_SOURCES	= mtr-setup.cc
@PACKAGE@_mtr_setup_LDADD	= $(top_builddir)/extensions/lib/libUniSetExtensions.la $(top_builddir)/lib/libUniSet.la
@PACKAGE@_mtr_setup_CXXFLAGS = -I$(top_builddir)/extensions/include

@PACKAGE@_mtr_read_SOURCES	= mtr-read.cc
@PACKAGE@_mtr_read_LDADD	= $(top_builddir)/extensions/lib/libUniSetExtensions.la $(top_builddir)/lib/libUniSet.la
@PACKAGE@_mtr_read_CXXFLAGS = -I$(top_builddir)/extensions/include

@PACKAGE@_vtconv_SOURCES	= vtconv.cc
@PACKAGE@_vtconv_LDADD	= $(top_builddir)/extensions/lib/libUniSetExtensions.la $(top_builddir)/lib/libUniSet.la
@PACKAGE@_vtconv_CXXFLAGS = -I$(top_builddir)/extensions/include

@PACKAGE@_rtu188_state_LDADD 		= libUniSetRTU.la libMBMaster.la \
		$(top_builddir)/extensions/lib/libUniSetExtensions.la $(top_builddir)/lib/libUniSet.la
@PACKAGE@_rtu188_state_CXXFLAGS	= -I$(top_builddir)/extensions/include
@PACKAGE@_rtu188_state_SOURCES		= rtustate.cc

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSetMBTCPMaster.pc libUniSetRTU.pc

all-local:
	ln -sf ../ModbusMaster/$(devel_include_HEADERS) ../include
