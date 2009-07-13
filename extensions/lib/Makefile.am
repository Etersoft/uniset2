lib_LTLIBRARIES 			= libUniSetExtensions.la
libUniSetExtensions_la_CPPFLAGS	= $(SIGC_CFLAGS) -I$(top_builddir)/extensions/include
libUniSetExtensions_la_LIBADD 	= $(SIGC_LIBS) $(top_builddir)/lib/libUniSet.la
libUniSetExtensions_la_SOURCES 	= Extensions.cc SMInterface.cc Calibration.cc SingleProcess.cc \
	IOBase.cc DigitalFilter.cc PID.cc MTR.cc VTypes.cc

#UObject_SK.cc 