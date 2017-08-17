#SUBDIRS=tests

bin_SCRIPTS = @PACKAGE@-codegen-go

xsldir 		= $(datadir)/@PACKAGE@/xslt
xsl_DATA 	= *.xsl

all-local:
	chmod 'a+x' @PACKAGE@-codegen-go


include $(top_builddir)/include.mk

test:
	./uniset2-codegen-go -l $(top_builddir)/wrappers/go/codegen -n TestGen testgen.src.xml
	go build testgen.go testgen_sk.go
