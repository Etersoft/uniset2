############################################################################
# This file is part of the UniSet library                                  #
############################################################################

if HAVE_EXTENTIONS
SUBDIRS = lib include SharedMemory IOControl LogicProcessor \
	ModbusMaster ModbusSlave SMViewer UniNetwork UNet2 DBServer-MySQL tests
#SMDBServer
#SharedMemoryPlus
#UDPExchange

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSetExtensions.pc

else
SUBDIRS =
endif

include $(top_builddir)/conf/common.mk



