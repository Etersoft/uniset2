noinst_PROGRAMS 	= test
test_LDADD 		= $(top_builddir)/lib/libUniSet.la
test_SOURCES 	= TestGen_SK.cc TestGen.cc TestGen-main.cc

TestGen_SK.cc TestGen-main.cc: ../@PACKAGE@-codegen testgen.src.xml ../*.xsl
	chmod 'a+x' ../@PACKAGE@-codegen
	./testgen.sh

clean-local:
	rm -rf *_SK.cc *_SK.h *-main.cc