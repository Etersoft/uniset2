noinst_PROGRAMS = mq-test
mq_test_LDADD = $(top_builddir)/lib/libUniSet2.la $(SIGC_LIBS) $(POCO_LIBS)
mq_test_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/extensions/include $(SIGC_CFLAGS) $(POCO_CFLAGS)
mq_test_SOURCES = TestProc_SK.cc TestProc.cc mq-test.cc

BUILT_SOURCES=TestProc_SK.cc TestProc_SK.h

TestProc_SK.cc TestProc_SK.h: testproc.src.xml
	$(UNISET_CODEGEN) -l $(abs_top_srcdir)/Utilities/codegen --ask -n TestProc --local-include --topdir $(top_srcdir)/ --no-main $(abs_srcdir)/testproc.src.xml

clean-local:
	rm -rf *_SK.cc *_SK.h
