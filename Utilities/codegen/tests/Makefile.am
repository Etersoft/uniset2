noinst_PROGRAMS 	= test
test_LDADD 		= $(top_builddir)/lib/libUniSet.la
test_SOURCES 	= TestGen_SK.cc TestGen.cc TestGen-main.cc

GENERATED=TestGen_SK.h TestGen_SK.cc TestGen-main.cc

$(GENERATED): ../@PACKAGE@-codegen testgen.src.xml ../*.xsl
	sh ../uniset-codegen --local-include -l --path ../ --ask -n TestGen testgen.src.xml

clean-local:
	rm -rf $(GENERATED)

all-local: $(GENERATED)
	
