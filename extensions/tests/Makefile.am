SUBDIR=SMemoryTest

noinst_PROGRAMS = calibration digitalfilter

calibration_SOURCES   = calibration.cc
calibration_LDADD     = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la
calibration_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include

digitalfilter_SOURCES  = digitalfilter.cc
digitalfilter_LDADD    = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la
digitalfilter_CPPFLAGS = -I$(top_builddir)/include -I$(top_builddir)/extensions/include

