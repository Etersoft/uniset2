############################################################################
# This file is part of the UniSet library								  #
############################################################################

FIRSTSUBDIRS=IDL
SUBDIRS=. src lib include Utilities tests extensions wrappers docs
# testsuite

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet2.pc

# Hack for fix build order
all-local:
	$(MAKE) -C $(FIRSTSUBDIRS) -j1 all

clean-local:
	$(MAKE) -C $(FIRSTSUBDIRS) clean

install-data-local:
	$(MAKE) -C $(FIRSTSUBDIRS) -j1 install

pvs-check:
	make clean
	pvs-studio-analyzer trace -- jmake
	pvs-studio-analyzer analyze -j$(nproc)
	plog-converter -a GA:1,2 -t tasklist -o uniset.pvs.tasks PVS-Studio.log

if HAVE_COVERAGE
COVERAGE_DIRS=$(COVERAGE_DEFAULT_DIRS)
include $(top_builddir)/testsuite/testcoverage-common.mk
endif

include $(top_builddir)/include.mk
