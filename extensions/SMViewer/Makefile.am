bin_PROGRAMS 	= @PACKAGE@-smviewer
@PACKAGE@_smviewer_LDADD		= $(SIGC_LIBS) $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la
@PACKAGE@_smviewer_CPPFLAGS	= $(SIGC_CFLAGS) -I$(top_builddir)/extensions/include
@PACKAGE@_smviewer_SOURCES = SMViewer.cc main.cc

include $(top_builddir)/include.mk
