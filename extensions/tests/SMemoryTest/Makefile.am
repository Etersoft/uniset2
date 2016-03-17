noinst_PROGRAMS = smemory-test		 
smemory_test_LDADD = $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(SIGC_LIBS) $(COMCPP_LIBS)
smemory_test_CPPFLAGS = -I$(top_builddir)/include \
	-I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory \
	$(SIGC_CFLAGS) $(COMCPP_CFLAGS)
smemory_test_SOURCES = TestProc_SK.cc TestProc.cc smemory-test.cc

BUILT_SOURCES=TestProc_SK.cc TestProc_SK.hh

TestProc_SK.cc: testproc.src.xml
#	./uniset2-codegen --ask -n TestProc --local-include --topdir $(top_builddir)/ --no-main testproc.src.xml
	$(top_builddir)/Utilities/codegen/@PACKAGE@-codegen -l $(top_builddir)/Utilities/codegen --ask -n TestProc --local-include --topdir $(top_builddir)/ --no-main testproc.src.xml

clean-local:
	rm -rf *_SK.cc *_SK.h

include $(top_builddir)/include.mk
