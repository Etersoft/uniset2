############################################################################
# This file is part of the UniSet library                                  #
############################################################################

UTILS = scripts Admin NullController SViewer-text \
	InfoServer SMonit MBTester codegen JrnTests
#MapSourceParser ClassGen

if DISABLE_MYSQL
SUBDIRS = ${UTILS}
else
SUBDIRS = ${UTILS} DBServer-MySQL
endif

include $(top_builddir)/conf/common.mk



