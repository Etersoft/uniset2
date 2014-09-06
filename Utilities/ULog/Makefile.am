bin_PROGRAMS = @PACKAGE@-log @PACKAGE@-log-stdin @PACKAGE@-logserver-wrap
noinst_PROGRAMS = @PACKAGE@-test-logserver

@PACKAGE@_test_logserver_SOURCES = logserver.cc
@PACKAGE@_test_logserver_LDADD = $(top_builddir)/lib/libUniSet2.la $(COMCPP_LIBS)
@PACKAGE@_test_logserver_CPPFLAGS = $(COMCPP_CFLAGS)

@PACKAGE@_log_SOURCES = log.cc
@PACKAGE@_log_LDADD = $(top_builddir)/lib/libUniSet2.la $(COMCPP_LIBS)
@PACKAGE@_log_CPPFLAGS = $(COMCPP_CFLAGS)

@PACKAGE@_log_stdin_SOURCES = log-stdin.cc
@PACKAGE@_log_stdin_LDADD = $(top_builddir)/lib/libUniSet2.la $(COMCPP_LIBS)
@PACKAGE@_log_stdin_CPPFLAGS = $(COMCPP_CFLAGS)

@PACKAGE@_logserver_wrap_SOURCES = log-wrap.cc
@PACKAGE@_logserver_wrap_LDADD = $(top_builddir)/lib/libUniSet2.la $(COMCPP_LIBS)
@PACKAGE@_logserver_wrap_CPPFLAGS = $(COMCPP_CFLAGS)
