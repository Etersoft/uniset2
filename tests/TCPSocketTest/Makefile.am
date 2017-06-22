noinst_PROGRAMS = sock-client sock-server
sock_client_LDADD = $(top_builddir)/lib/libUniSet2.la $(SIGC_LIBS) $(POCO_LIBS)
sock_client_CPPFLAGS = -I$(top_builddir)/include -I$(top_builddir)/extensions/include $(SIGC_CFLAGS) $(POCO_CFLAGS)
sock_client_SOURCES = sock-client.cc

sock_server_LDADD = $(top_builddir)/lib/libUniSet2.la $(SIGC_LIBS) $(POCO_LIBS)
sock_server_CPPFLAGS = -I$(top_builddir)/include -I$(top_builddir)/extensions/include $(SIGC_CFLAGS) $(POCO_CFLAGS)
sock_server_SOURCES = sock-server.cc
