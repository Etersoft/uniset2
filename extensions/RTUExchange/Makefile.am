bin_PROGRAMS 			= @PACKAGE@-rtuexchange mtr-conv uniset-rtu188-state vtconv mtr-setup

# не забывайте править версию в pc-файле
URTU_VER=2:0:0

lib_LTLIBRARIES 			= libUniSetRTU.la
libUniSetRTU_la_LDFLAGS  = -version-info $(URTU_VER) 
libUniSetRTU_la_LIBADD		= $(top_builddir)/lib/libUniSet.la \
								$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
								$(top_builddir)/extensions/lib/libUniSetExtensions.la \
								$(SIGC_LIBS)
libUniSetRTU_la_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)
libUniSetRTU_la_SOURCES 	= RTUStorage.cc RTUExchange.cc

@PACKAGE@_rtuexchange_SOURCES 	= rtuexchange.cc
@PACKAGE@_rtuexchange_LDADD 	= libUniSetRTU.la $(top_builddir)/lib/libUniSet.la \
									$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSetExtensions.la \
									$(SIGC_LIBS)
@PACKAGE@_rtuexchange_CXXFLAGS	= -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS)

mtr_conv_SOURCES	= mtrconv.cc
mtr_conv_LDADD	= $(top_builddir)/extensions/lib/libUniSetExtensions.la $(top_builddir)/lib/libUniSet.la
mtr_conv_CXXFLAGS = -I$(top_builddir)/extensions/include

mtr_setup_SOURCES	= mtr-setup.cc
mtr_setup_LDADD	= $(top_builddir)/extensions/lib/libUniSetExtensions.la $(top_builddir)/lib/libUniSet.la
mtr_setup_CXXFLAGS = -I$(top_builddir)/extensions/include

vtconv_SOURCES	= vtconv.cc
vtconv_LDADD	= $(top_builddir)/extensions/lib/libUniSetExtensions.la $(top_builddir)/lib/libUniSet.la
vtconv_CXXFLAGS = -I$(top_builddir)/extensions/include

uniset_rtu188_state_LDADD 		= libUniSetRTU.la $(top_builddir)/extensions/lib/libUniSetExtensions.la $(top_builddir)/lib/libUniSet.la
uniset_rtu188_state__CXXFLAGS	= -I$(top_builddir)/extensions/include
uniset_rtu188_state_SOURCES		= rtustate.cc

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSetRTU.pc

all-local:
	ln -sf ../RTUExchange/$(devel_include_HEADERS) ../include
