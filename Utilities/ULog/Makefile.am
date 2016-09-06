bin_PROGRAMS = @PACKAGE@-log  @PACKAGE@-logserver-wrap @PACKAGE@-log2val
noinst_PROGRAMS = @PACKAGE@-test-logserver @PACKAGE@-log-stdin

@PACKAGE@_test_logserver_SOURCES = logserver.cc
@PACKAGE@_test_logserver_LDADD = $(top_builddir)/lib/libUniSet2.la $(POCO_LIBS)
@PACKAGE@_test_logserver_CPPFLAGS = $(POCO_CFLAGS)

@PACKAGE@_log_SOURCES = log.cc
@PACKAGE@_log_LDADD = $(top_builddir)/lib/libUniSet2.la $(POCO_LIBS)
@PACKAGE@_log_CPPFLAGS = $(POCO_CFLAGS)

@PACKAGE@_log_stdin_SOURCES = log-stdin.cc
@PACKAGE@_log_stdin_LDADD = $(top_builddir)/lib/libUniSet2.la $(POCO_LIBS)
@PACKAGE@_log_stdin_CPPFLAGS = $(POCO_CFLAGS)

@PACKAGE@_logserver_wrap_SOURCES = log-wrap.cc
@PACKAGE@_logserver_wrap_LDADD = $(top_builddir)/lib/libUniSet2.la $(POCO_LIBS)
@PACKAGE@_logserver_wrap_CPPFLAGS = $(POCO_CFLAGS)

@PACKAGE@_log2val_SOURCES = log2val.cc
@PACKAGE@_log2val_LDADD = $(top_builddir)/lib/libUniSet2.la

include $(top_builddir)/include.mk
