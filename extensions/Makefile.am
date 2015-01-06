############################################################################
# This file is part of the UniSet library                                  #
############################################################################

if HAVE_EXTENTIONS
SUBDIRS = lib include SharedMemory IOControl LogicProcessor \
	ModbusMaster ModbusMaster/tests ModbusSlave SMViewer UniNetwork UNetUDP UNetUDP/tests DBServer-MySQL DBServer-SQLite \
	RRDServer SharedMemoryPlus tests ModbusSlave/tests

#SMDBServer
#SharedMemoryPlus

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet2Extensions.pc

else
SUBDIRS =
endif
