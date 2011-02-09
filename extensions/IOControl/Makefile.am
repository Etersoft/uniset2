bin_PROGRAMS = @PACKAGE@-iocontrol @PACKAGE@-iotest @PACKAGE@-iocalibr

# не забывайте править версию в pc-файле 
UIO_VER=@LIBVER@

lib_LTLIBRARIES 	= libUniSetIOControl.la
libUniSetIOControl_la_LDFLAGS  = -version-info $(UIO_VER) 
libUniSetIOControl_la_LIBADD		= $(COMEDI_LIBS) $(SIGC_LIBS) \
		$(top_builddir)/extensions/lib/libUniSetExtensions.la \
		$(top_builddir)/lib/libUniSet.la \
		$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la

libUniSetIOControl_la_CXXFLAGS 	= $(COMEDI_CFLAGS) $(SIGC_CFLAGS) \
	-I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory

libUniSetIOControl_la_SOURCES 	= ComediInterface.cc IOControl.cc

@PACKAGE@_iocontrol_LDADD 	= libUniSetIOControl.la $(COMEDI_LIBS)
@PACKAGE@_iocontrol_CXXFLAGS 	= $(COMEDI_CFLAGS) $(SIGC_CFLAGS) \
	-I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory
@PACKAGE@_iocontrol_SOURCES 	= iocontrol.cc

@PACKAGE@_iotest_CXXFLAGS = $(COMEDI_CFLAGS)
@PACKAGE@_iotest_LDADD 	= $(COMEDI_LIBS)
@PACKAGE@_iotest_SOURCES 	= iotest.cc

@PACKAGE@_iocalibr_CXXFLAGS 	= $(COMEDI_CFLAGS)
@PACKAGE@_iocalibr_LDADD 		= $(COMEDI_LIBS) \
	$(top_builddir)/lib/libUniSet.la \
	$(top_builddir)/extensions/lib/libUniSetExtensions.la
@PACKAGE@_iocalibr_SOURCES 	= iocalibr.cc

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSetIOControl.pc

all-local:
	ln -sf ../IOControl/$(devel_include_HEADERS) ../include
