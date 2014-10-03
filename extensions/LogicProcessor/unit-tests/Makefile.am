noinst_PROGRAMS = tests

tests_SOURCES   = tests.cc lproc.cc
tests_LDADD     = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/LogicProcessor/libUniSet2LProcessor.la
tests_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include -I$(top_builddir)/extensions/LogicProcessor \
-I$(includedir)/Catch
