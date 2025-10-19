if ENABLE_JSCRIPT

noinst_LTLIBRARIES = libUniSet2JScript.la
libUniSet2JScript_la_LIBADD = $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la $(SIGC_LIBS)
libUniSet2JScript_la_CXXFLAGS = -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(QUICKJS_CFLAGS)
libUniSet2JScript_la_SOURCES = JSProxy_SK.cc JSProxy.cc JSEngine.cc JSHelpers.cc

bin_PROGRAMS = @PACKAGE@-jscript

@PACKAGE@_jscript_SOURCES = main.cc
@PACKAGE@_jscript_LDADD = libUniSet2JScript.la  \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/lib/libUniSet2.la $(SIGC_LIBS) $(QUICKJS_LIBS)
@PACKAGE@_jscript_CXXFLAGS = -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory \
	$(SIGC_CFLAGS) $(QUICKJS_CFLAGS)

JSProxy_SK.cc JSProxy_SK.h: $(top_builddir)/Utilities/codegen/@PACKAGE@-codegen jsproxy.src.xml
	$(SHELL) $(top_builddir)/Utilities/codegen/@PACKAGE@-codegen -l $(top_builddir)/Utilities/codegen --no-main -n JSProxy jsproxy.src.xml

# install
jsdir = $(datadir)/@PACKAGE@/js
js_DATA = js/*.js

# SUBDIRS=tests

include $(top_builddir)/include.mk

endif
