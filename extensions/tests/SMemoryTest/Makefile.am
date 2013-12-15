noinst_PROGRAMS = smemory-test
smemory_test_LDADD = $(top_builddir)/lib/libUniSet.la \
	$(top_builddir)/extensions/lib/libUniSetExtensions.la \
	$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
	$(SIGC_LIBS)
smemory_test_CPPFLAGS = -I$(top_builddir)/include \
	-I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory \
	$(SIGC_CFLAGS)
smemory_test_SOURCES = TestProc_SK.cc TestProc.cc smemory-test.cc

TestProc_SK.cc: testproc.src.xml
#	./uniset-codegen --ask -n TestProc --local-include --topdir $(top_builddir)/ --no-main testproc.src.xml
	$(top_builddir)/Utilities/codegen/@PACKAGE@-codegen -l $(top_builddir)/Utilities/codegen --ask -n TestProc --local-include --topdir $(top_builddir)/ --no-main testproc.src.xml

clean-local:
	rm -rf *_SK.cc *_SK.h
