noinst_PROGRAMS = mq-test
mq_test_LDADD = $(top_builddir)/lib/libUniSet2.la $(SIGC_LIBS) $(COMCPP_LIBS)
mq_test_CPPFLAGS = -I$(top_builddir)/include -I$(top_builddir)/extensions/include $(SIGC_CFLAGS) $(COMCPP_CFLAGS)
mq_test_SOURCES = TestProc_SK.cc TestProc.cc mq-test.cc

BUILT_SOURCES=TestProc_SK.cc TestProc_SK.h

TestProc_SK.cc TestProc_SK.h: testproc.src.xml
	$(top_builddir)/Utilities/codegen/@PACKAGE@-codegen -l $(top_builddir)/Utilities/codegen --ask -n TestProc --local-include --topdir $(top_builddir)/ --no-main testproc.src.xml

clean-local:
	rm -rf *_SK.cc *_SK.h
