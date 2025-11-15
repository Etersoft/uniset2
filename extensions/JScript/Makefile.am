if ENABLE_JSCRIPT

noinst_LTLIBRARIES = libUniSet2JScript.la
if ENABLE_OPCUA
noinst_LTLIBRARIES += libUniSet2JScriptOpcua.la
endif

libUniSet2JScript_la_LIBADD = $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la $(SIGC_LIBS)
libUniSet2JScript_la_CXXFLAGS = -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(QUICKJS_CFLAGS)

common_js_sources = JSProxy_SK.cc JSProxy.cc JSEngine.cc JSHelpers.cc JHttpServer.cc JSModbusClient.cc
libUniSet2JScript_la_SOURCES = $(common_js_sources)

if ENABLE_OPCUA
libUniSet2JScriptOpcua_la_LIBADD = $(libUniSet2JScript_la_LIBADD)
libUniSet2JScriptOpcua_la_CXXFLAGS = $(libUniSet2JScript_la_CXXFLAGS) $(OPEN62541_CFLAGS) $(OPEN62541PP_CFLAGS) -DJS_OPCUA_ENABLED
libUniSet2JScriptOpcua_la_SOURCES = $(common_js_sources) JSOPCUAClient.cc
endif

if ENABLE_OPCUA
bin_PROGRAMS = @PACKAGE@-jscript @PACKAGE@-jscript-opcua
else
bin_PROGRAMS = @PACKAGE@-jscript
endif

@PACKAGE@_jscript_SOURCES = main.cc
@PACKAGE@_jscript_LDADD = libUniSet2JScript.la  \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/lib/libUniSet2.la $(SIGC_LIBS) $(QUICKJS_LIBS)
@PACKAGE@_jscript_CXXFLAGS = -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory \
	$(SIGC_CFLAGS) $(QUICKJS_CFLAGS)

if ENABLE_OPCUA
@PACKAGE@_jscript_opcua_SOURCES = main.cc
@PACKAGE@_jscript_opcua_LDADD = libUniSet2JScriptOpcua.la  \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/lib/libUniSet2.la $(SIGC_LIBS) $(QUICKJS_LIBS) \
	$(OPEN62541_LIBS) $(OPEN62541PP_LIBS)
@PACKAGE@_jscript_opcua_CXXFLAGS = -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory \
	$(SIGC_CFLAGS) $(QUICKJS_CFLAGS)
endif

JSProxy_SK.cc JSProxy_SK.h: $(top_builddir)/Utilities/codegen/@PACKAGE@-codegen jsproxy.src.xml
	$(SHELL) $(top_builddir)/Utilities/codegen/@PACKAGE@-codegen -l $(top_builddir)/Utilities/codegen --no-main -n JSProxy jsproxy.src.xml

# install
jsdir = $(datadir)/@PACKAGE@/js
js_DATA = js/*.js

# SUBDIRS=tests

include $(top_builddir)/include.mk

endif
