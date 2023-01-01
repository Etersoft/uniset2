noinst_PROGRAMS = opcua-server opcua-client

opcua_server_LDADD = $(top_builddir)/lib/libUniSet2.la $(SIGC_LIBS) $(OPEN62541_LIBS)
opcua_server_CPPFLAGS = -I$(top_builddir)/include -I$(top_builddir)/extensions/include $(SIGC_CFLAGS) $(OPEN62541_LIBS)
opcua_server_SOURCES = server.cc

opcua_client_LDADD = $(top_builddir)/lib/libUniSet2.la $(SIGC_LIBS) $(OPEN62541_LIBS)
opcua_client_CPPFLAGS = -I$(top_builddir)/include -I$(top_builddir)/extensions/include $(SIGC_CFLAGS) $(OPEN62541_LIBS)
opcua_client_SOURCES = client.cc