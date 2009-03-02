#noinst_PROGRAMS 	= test
#test_LDADD 		= $(top_builddir)/lib/libUniSet.la
#test_SOURCES 	= TestGen_SK.cc TestGen.cc TestGen-main.cc

TestGen_SK.cc: @PACKAGE@-codegen testgen.src.xml
	chmod 'a+x' @PACKAGE@-codegen
	./uniset-codegen --local-include -l --ask -n TestGen testgen.src.xml

clean-local:
	rm -rf *_SK.cc *_SK.h *-main.cc