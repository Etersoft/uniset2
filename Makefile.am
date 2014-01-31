############################################################################
# This file is part of the UniSet library                                  #
############################################################################

FIRSTSUBDIRS=IDL
SUBDIRS=. src lib include Utilities extensions docs

include $(top_builddir)/conf/common.mk

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet.pc

# Hack for fix build order
all-local:
	$(MAKE) -C $(FIRSTSUBDIRS) -j1 all

clean-local:
	$(MAKE) -C $(FIRSTSUBDIRS) clean

install-data-local:
	$(MAKE) -C $(FIRSTSUBDIRS) -j1 install
