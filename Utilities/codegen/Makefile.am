#SUBDIRS=tests

bin_SCRIPTS = @PACKAGE@-codegen

xsldir 		= $(datadir)/@PACKAGE@/xslt
xsl_DATA 	= $(wildcard $(srcdir)/*.xsl $(srcdir)/skel*)

all-local:
	chmod 'a+x' @PACKAGE@-codegen

				   
include $(top_builddir)/include.mk
