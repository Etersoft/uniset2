bin_PROGRAMS 	= @PACKAGE@-smviewer
@PACKAGE@_smviewer_LDADD		= $(SIGC_LIBS) $(top_builddir)/lib/libUniSet.la \
	$(top_builddir)/extentions/lib/libUniSetExtentions.la
@PACKAGE@_smviewer_CPPFLAGS	= $(SIGC_CFLAGS) -I$(top_builddir)/extentions/include
@PACKAGE@_smviewer_SOURCES = SMViewer.cc main.cc
