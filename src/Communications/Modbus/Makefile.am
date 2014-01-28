############################################################################
# This file is part of the UniSet library                                  #
############################################################################
noinst_LTLIBRARIES 		= libModbus.la
libModbus_la_SOURCES	= ModbusTypes.cc ModbusHelpers.cc \
	ModbusClient.cc ModbusServer.cc ModbusServerSlot.cc \
	ModbusRTUSlave.cc ModbusRTUSlaveSlot.cc ModbusRTUMaster.cc \
	ModbusTCPCore.cc ModbusTCPServer.cc ModbusTCPServerSlot.cc ModbusTCPMaster.cc TCPCheck.cc
	
libModbus_la_CXXFLAGS	= -I$(top_builddir)/include/Communications/modbus $(SIGC_CFLAGS) $(COMCPP_CFLAGS)
libModbus_la_LIBADD		= $(SIGC_LIBS) $(COMCPP_LIBS)
