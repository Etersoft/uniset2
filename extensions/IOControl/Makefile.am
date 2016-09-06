if ENABLE_IO
bin_PROGRAMS = @PACKAGE@-iocontrol @PACKAGE@-iotest @PACKAGE@-iocalibr

# не забывайте править версию в2.pc-файле
UIO_VER=@LIBVER@

lib_LTLIBRARIES 	= libUniSet2IOControl.la
libUniSet2IOControl_la_LDFLAGS = -version-info $(UIO_VER)
libUniSet2IOControl_la_LIBADD  = $(COMEDI_LIBS) $(SIGC_LIBS) $(POCO_LIBS) \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	-lm

libUniSet2IOControl_la_CXXFLAGS = $(COMEDI_CFLAGS) $(SIGC_CFLAGS) $(POCO_CFLAGS) \
	-I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory

libUniSet2IOControl_la_SOURCES = ComediInterface.cc IOControl.cc

@PACKAGE@_iocontrol_LDADD = libUniSet2IOControl.la \
	$(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(COMEDI_LIBS) $(SIGC_LIBS) $(POCO_LIBS) -lm
@PACKAGE@_iocontrol_CXXFLAGS = $(COMEDI_CFLAGS) $(SIGC_CFLAGS) $(POCO_CFLAGS) \
	-I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory
@PACKAGE@_iocontrol_SOURCES = iocontrol.cc

@PACKAGE@_iotest_CXXFLAGS = $(COMEDI_CFLAGS)
@PACKAGE@_iotest_LDADD 	= $(COMEDI_LIBS) -lm
@PACKAGE@_iotest_SOURCES 	= iotest.cc

@PACKAGE@_iocalibr_CXXFLAGS = $(COMEDI_CFLAGS)
@PACKAGE@_iocalibr_LDADD 		= $(COMEDI_LIBS) \
	$(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	-lm
@PACKAGE@_iocalibr_SOURCES 	= iocalibr.cc

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions/io

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet2IOControl.pc

#all-local:
#	ln -sf ../IOControl/$(devel_include_HEADERS) ../include

include $(top_builddir)/include.mk

endif
# end of ENABLE_IO
