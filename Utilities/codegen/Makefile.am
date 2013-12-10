bin_PROGRAMS = UObject
UObject_SOURCES = UObject_SK.cc UObject.cc

UObject_SK.cc: UObject.src.xml
	@UNISET_CODEGEN@ --ask -n UObject --topdir $(top_builddir)/ --no-main UObject.src.xml

clean-local:
	rm -rf *_SK.cc *_SK.h

all-local: UObject_SK.cc UObject_SK.h
	
