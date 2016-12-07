############################################################################
# This file is part of the UniSet library								  #
############################################################################
#LT_CURRENT=0
#LT_REVISION=0
#LT_AGE=0
#LT_RELEASE=0.99
#libUniSet2_la_LDFLAGS	= -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -release $(LT_RELEASE)
# `0:0:0' (CURRENT=0, REVISION-0, AGE=0). Формат версии C:R:A.
# -release 1

lib_LTLIBRARIES 		= libUniSet2.la
libUniSet2_la_SOURCES	=
libUniSet2_la_LDFLAGS	= -version-info @LIBVER@
libUniSet2_la_LIBADD	= -lm \
 $(top_builddir)/src/Communications/libCommunications.la \
 $(top_builddir)/src/Communications/Modbus/libModbus.la \
 $(top_builddir)/src/Communications/TCP/libTCP.la \
 $(top_builddir)/src/Communications/Http/libHttp.la \
 $(top_builddir)/src/Interfaces/libInterfaces.la \
 $(top_builddir)/src/ObjectRepository/libObjectsRepository.la \
 $(top_builddir)/src/Processes/libProcesses.la \
 $(top_builddir)/src/Services/libServices.la \
 $(top_builddir)/src/Timers/libTimers.la \
 $(top_builddir)/src/Various/libVarious.la \
 $(top_builddir)/src/Log/libLog.la

include $(top_builddir)/include.mk
