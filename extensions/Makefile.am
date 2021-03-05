############################################################################
# This file is part of the UniSet library								  #
############################################################################

if HAVE_EXTENTIONS
SUBDIRS = lib include SharedMemory SharedMemory/tests IOControl IOControl/tests LogicProcessor LogicProcessor/tests \
	ModbusMaster  ModbusSlave  SMViewer UniNetwork UNetUDP UNetUDP/tests \
	DBServer-MySQL DBServer-SQLite DBServer-PostgreSQL MQTTPublisher \
	RRDServer tests ModbusMaster/tests ModbusSlave/tests LogDB LogDB/tests \
<<<<<<< HEAD
	Backend-OpenTSDB HttpResolver HttpResolver/tests UWebSocketGate
=======
	Backend-OpenTSDB HttpResolver HttpResolver/tests UWebSocketGate UWebSocketGate/tests
>>>>>>> 2.9.4-alt0.1


pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet2Extensions.pc

else
SUBDIRS =
endif
