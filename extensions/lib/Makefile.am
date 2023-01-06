# не забывайте править версию в2.pc-файле
UEXT_VER=@LIBVER@

lib_LTLIBRARIES = libUniSet2Extensions.la
libUniSet2Extensions_la_LDFLAGS  = -version-info $(UEXT_VER)
libUniSet2Extensions_la_CPPFLAGS = $(SIGC_CFLAGS) $(POCO_CFLAGS) -I$(top_srcdir)/extensions/include
libUniSet2Extensions_la_LIBADD   = $(SIGC_LIBS) $(POCO_LIBS) $(top_builddir)/lib/libUniSet2.la
libUniSet2Extensions_la_SOURCES  = Extensions.cc SMInterface.cc Calibration.cc \
	IOBase.cc DigitalFilter.cc PID.cc MTR.cc VTypes.cc UObject_SK.cc

UObject_SK.cc: $(top_srcdir)/Utilities/codegen/*.xsl
	$(SHEL) $(top_srcdir)/Utilities/codegen/uniset2-codegen -l $(top_srcdir)/Utilities/codegen -n UObject --no-main $(top_srcdir)/Utilities/codegen/tests/uobject.src.xml
	mv -f UObject_SK.h $(abs_top_builddir)/include/

include $(top_builddir)/include.mk
