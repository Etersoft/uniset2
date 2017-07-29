############################################################################
# This file is part of the UniSet library								  #
############################################################################

if HAVE_EXTENTIONS
SUBDIRS = lib include SharedMemory SharedMemory/tests IOControl LogicProcessor LogicProcessor/tests \
	ModbusMaster  ModbusSlave  SMViewer UniNetwork UNetUDP UNetUDP/tests \
	DBServer-MySQL DBServer-SQLite DBServer-PostgreSQL MQTTPublisher \
	RRDServer tests ModbusMaster/tests ModbusSlave/tests

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet2Extensions.pc

else
SUBDIRS =
endif
