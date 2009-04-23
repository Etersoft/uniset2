bin_PROGRAMS 	= @PACKAGE@-sm-tsviewer
@PACKAGE@_sm_tsviewer_LDADD		= $(SIGC_LIBS) $(top_builddir)/lib/libUniSet.la \
	$(top_builddir)/extentions/lib/libUniSetExtentions.la
@PACKAGE@_sm_tsviewer_CPPFLAGS	= $(SIGC_CFLAGS) -I$(top_builddir)/extentions/include
@PACKAGE@_sm_tsviewer_SOURCES = SMViewer.cc main.cc
