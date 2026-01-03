############################################################################
# This file is part of the UniSet library                                 #
############################################################################

# Library for tests and external use
noinst_LTLIBRARIES = libUniSet2Launcher.la

libUniSet2Launcher_la_SOURCES = \
	ProcessInfo.cc \
	DependencyResolver.cc \
	HealthChecker.cc \
	ProcessManager.cc \
	ConfigLoader.cc \
	LauncherHttpRegistry.cc \
	ProcessTemplate.cc

libUniSet2Launcher_la_LIBADD = $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(SIGC_LIBS) $(POCO_LIBS)

libUniSet2Launcher_la_CPPFLAGS = $(SIGC_CFLAGS) $(POCO_CFLAGS) \
	-I$(top_builddir)/extensions/include

# Binary
bin_PROGRAMS = @PACKAGE@-launcher

@PACKAGE@_launcher_SOURCES = main.cc

@PACKAGE@_launcher_LDADD = libUniSet2Launcher.la \
	$(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(SIGC_LIBS) $(POCO_LIBS)

@PACKAGE@_launcher_CPPFLAGS = $(SIGC_CFLAGS) $(POCO_CFLAGS) \
	-I$(top_builddir)/extensions/include \
	-DUNISET_DATADIR=\"$(datadir)/@PACKAGE@\"

# Web UI files
launcherwebdir = $(datadir)/@PACKAGE@
launcherweb_DATA = launcher.html launcher-app.js

EXTRA_DIST = launcher.html launcher-app.js

include $(top_builddir)/include.mk
