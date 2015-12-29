############################################################################
# This file is part of the UniSet library								  #
############################################################################
noinst_LTLIBRARIES 		= libModbus.la
libModbus_la_SOURCES	= ModbusTypes.cc ModbusHelpers.cc ModbusTCPSession.cc \
	ModbusClient.cc ModbusServer.cc ModbusServerSlot.cc \
	ModbusRTUSlave.cc ModbusRTUSlaveSlot.cc ModbusRTUMaster.cc \
	ModbusTCPCore.cc ModbusTCPServer.cc ModbusTCPServerSlot.cc ModbusTCPMaster.cc
	
libModbus_la_CXXFLAGS	= -I$(top_builddir)/include/Communications/modbus $(SIGC_CFLAGS) $(COMCPP_CFLAGS) $(EV_CFLAGS)
#libModbus_la_LIBADD		= $(top_builddir)/src/Communications/TCP/libTCP.la $(SIGC_LIBS) $(COMCPP_LIBS)
libModbus_la_LIBADD		= $(SIGC_LIBS) $(COMCPP_LIBS) $(EV_LIBS)

include $(top_builddir)/include.mk
