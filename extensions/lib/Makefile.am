# не забывайте править версию в2.pc-файле
UEXT_VER=@LIBVER@

lib_LTLIBRARIES 			= libUniSet2Extensions.la
libUniSet2Extensions_la_LDFLAGS  = -version-info $(UEXT_VER)
libUniSet2Extensions_la_CPPFLAGS = $(SIGC_CFLAGS) -I$(top_builddir)/extensions/include
libUniSet2Extensions_la_LIBADD   = $(SIGC_LIBS) $(top_builddir)/lib/libUniSet2.la
libUniSet2Extensions_la_SOURCES  = Extensions.cc SMInterface.cc Calibration.cc SingleProcess.cc \
	IOBase.cc DigitalFilter.cc PID.cc MTR.cc VTypes.cc UObject_SK.cc

include $(top_builddir)/include.mk
