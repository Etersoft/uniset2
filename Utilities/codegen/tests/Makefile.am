noinst_PROGRAMS = test test2 test3
#test2

test_LDADD 		= $(top_builddir)/lib/libUniSet2.la $(POCO_LIBS)
test_CXXFLAGS	= -I$(top_builddir)/include $(POCO_CGLAGS) -Wno-unused-function
test_SOURCES 	= TestGen_SK.cc TestGen.cc TestGen-main.cc

test2_LDADD 	= $(top_builddir)/lib/libUniSet2.la $(POCO_LIBS)
test2_CXXFLAGS	= -I$(top_builddir)/include $(POCO_CGLAGS) -Wno-unused-function
test2_SOURCES 	= TestGenAlone_SK.cc TestGenAlone.cc TestGenAlone-main.cc

test3_LDADD 	= $(top_builddir)/lib/libUniSet2.la $(POCO_LIBS)
test3_CXXFLAGS	= -I$(top_builddir)/include $(POCO_CGLAGS) -Wno-unused-function
test3_SOURCES 	= TestGenSkel_SK.cc TestGenSkel.cc TestGenSkel-main.cc

GENERATED=TestGen_SK.h TestGen_SK.cc TestGen-main.cc
GENERATED2=TestGenAlone_SK.h TestGenAlone_SK.cc TestGenAlone-main.cc
GENERATED3=TestGenSkel_SK.h TestGenSkel_SK.cc TestGenSkel-main.cc
GENUOBJ=UObject_SK.cc UObject_SK.h

BUILT_SOURCES=$(GENERATED) $(GENERATED2) $(GENOBJ)

TestGen-main.cc TestGen_SK.cc TestGen_SK.h: ../@PACKAGE@-codegen testgen.src.xml ../*.xsl
	$(SHELL) ../@PACKAGE@-codegen -l $(top_builddir)/Utilities/codegen --gen-varmap --local-include -n TestGen testgen.src.xml

TestGenAlone-main.cc TestGenAlone_SK.cc TestGenAlone_SK.h: ../@PACKAGE@-codegen testgen-alone.src.xml ../*.xsl
	$(SHELL) ../@PACKAGE@-codegen -l $(top_builddir)/Utilities/codegen --local-include --alone -n TestGenAlone testgen-alone.src.xml
	
TestGenSkel-main.cc TestGenSkel_SK.cc TestGenSkel_SK.h: ../@PACKAGE@-codegen ../*.xsl ../skel*
	$(SHELL) ../@PACKAGE@-codegen -l $(top_builddir)/Utilities/codegen --local-include --make-skel TestGenSkel

$(GENUOBJ): ../@PACKAGE@-codegen uobject.src.xml ../*.xsl
	$(SHELL) ../@PACKAGE@-codegen -l $(top_builddir)/Utilities/codegen -n UObject --no-main uobject.src.xml

clean-local:
	rm -rf $(GENERATED) $(GENERATED2) $(GENERATED3) $(GENUOBJ)

all-local: $(GENERATED) $(GENERATED2) $(GENERATED3) $(GENUOBJ)
	
	
