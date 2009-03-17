############################################################################
# This file is part of the UniSet library                                  #
############################################################################

if HAVE_EXTENTIONS
SUBDIRS = lib include SharedMemory IOControl RTUExchange LogicProcessor \
	ModbusSlave MBTCPMaster
#SharedMemoryPlus
#UDPExchange

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSetExtentions.pc

else
SUBDIRS =
endif

include $(top_builddir)/conf/common.mk



