if HAVE_TESTS

noinst_PROGRAMS = tests

BUILT_SOURCES = TestObject_SK.cc TestObject_SK.h

TestObject_SK.cc TestObject_SK.h: testobject.src.xml
	$(UNISET_CODEGEN) -l $(top_builddir)/Utilities/codegen -n TestObject --topdir $(top_builddir)/ --ask --no-main testobject.src.xml

tests_SOURCES = TestObject_SK.cc NullSM.cc TestObject.cc test_sm.cc tests.cc
tests_LDADD	 = $(top_builddir)/lib/libUniSet2.la $(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(SIGC_LIBS) $(POCO_LIBS) -lpthread
tests_CPPFLAGS  = -I$(top_builddir)/include -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory $(SIGC_CFLAGS) $(POCO_CFLAGS)

include $(top_builddir)/testsuite/testsuite-common.mk

check-local: atconfig package.m4 $(TESTSUITE) sm-tests.at
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)

clean-local:
	rm -rf $(CLEANFILES)
	rm -rf $(COVERAGE_REPORT_DIR)
	rm -rf *_SK.*

include $(top_builddir)/include.mk

endif
