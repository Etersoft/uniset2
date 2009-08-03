############################################################################
# This file is part of the UniSet library                                  #
############################################################################

FIRSTSUBDIRS=IDL
SUBDIRS=. src lib include Utilities extensions docs

include $(top_builddir)/conf/common.mk

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet.pc

all-local:
	$(MAKE) -C $(FIRSTSUBDIRS)
