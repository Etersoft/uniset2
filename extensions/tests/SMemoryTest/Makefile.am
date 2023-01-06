noinst_PROGRAMS = smemory-test sm-lostmessage-test
smemory_test_LDADD = $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(SIGC_LIBS) $(POCO_LIBS)
smemory_test_CPPFLAGS = -I$(top_srcdir)/include \
	-I$(top_srcdir)/extensions/include \
	-I$(top_srcdir)/extensions/SharedMemory \
	$(SIGC_CFLAGS) $(POCO_CFLAGS)
smemory_test_SOURCES = TestProc_SK.cc TestProc.cc smemory-test.cc

BUILT_SOURCES=TestProc_SK.cc TestProc_SK.h LostTestProc_SK.cc LostTestProc_SK.h

TestProc_SK.cc TestProc_SK.h: testproc.src.xml
	$(UNISET_CODEGEN) -l $(abs_top_srcdir)/Utilities/codegen --ask -n TestProc --local-include --topdir $(top_srcdir)/ --no-main $(abs_srcdir)/testproc.src.xml

LostTestProc_SK.cc LostTestProc_SK.h: losttestproc.src.xml
	$(UNISET_CODEGEN) -l $(abs_top_srcdir)/Utilities/codegen --ask -n LostTestProc --local-include --topdir $(top_srcdir)/ --no-main $(abs_srcdir)/losttestproc.src.xml

sm_lostmessage_test_LDADD = $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(SIGC_LIBS) $(POCO_LIBS)
sm_lostmessage_test_CPPFLAGS = -I$(top_srcdir)/include \
	-I$(top_srcdir)/extensions/include \
	-I$(top_srcdir)/extensions/SharedMemory \
	$(SIGC_CFLAGS) $(POCO_CFLAGS)
sm_lostmessage_test_SOURCES = LostTestProc_SK.cc LostTestProc.cc LostPassiveTestProc.cc sm-lostmessage-test.cc


clean-local:
	rm -rf *_SK.cc *_SK.h

include $(top_builddir)/include.mk
