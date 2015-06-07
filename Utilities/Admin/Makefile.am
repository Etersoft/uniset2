############################################################################
# This file is part of the UniSet library								  #
############################################################################

bin_SCRIPTS = @PACKAGE@-vmonit

bin_PROGRAMS = @PACKAGE@-admin
@PACKAGE@_admin_SOURCES  = admin.cc
@PACKAGE@_admin_LDADD	= $(top_builddir)/lib/libUniSet2.la
@PACKAGE@_admin_CPPFLAGS = -I$(top_builddir)/lib

include $(top_builddir)/include.mk
