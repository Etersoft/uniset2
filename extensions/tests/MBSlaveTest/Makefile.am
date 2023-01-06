noinst_PROGRAMS = mbslave-test
mbslave_test_LDADD = $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(top_builddir)/extensions/ModbusSlave/libUniSet2MBSlave.la \
	$(SIGC_LIBS) $(POCO_LIBS)
mbslave_test_CPPFLAGS = -I$(top_srcdir)/include \
	-I$(top_srcdir)/extensions/include \
	-I$(top_srcdir)/extensions/SharedMemory \
	-I$(top_srcdir)/extensions/ModbusSlave \
	$(SIGC_CFLAGS) $(POCO_CFLAGS)
mbslave_test_SOURCES = TestProc_SK.cc TestProc.cc mbslave-test.cc

BUILT_SOURCES=TestProc_SK.cc TestProc_SK.h

TestProc_SK.cc TestProc_SK.h: testproc.src.xml
	$(UNISET_CODEGEN) -l $(abs_top_srcdir)/Utilities/codegen --ask -n TestProc --local-include --topdir $(top_srcdir)/ --no-main $(abs_srcdir)/testproc.src.xml

clean-local:
	rm -rf *_SK.cc *_SK.h

include $(top_builddir)/include.mk
