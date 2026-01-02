############################################################################
# This file is part of the UniSet library                                 #
############################################################################

bin_PROGRAMS = @PACKAGE@-uno

@PACKAGE@_uno_SOURCES = main.cc UniSetUno.cc

@PACKAGE@_uno_LDADD = $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(top_builddir)/extensions/UNetUDP/libUniSet2UNetUDP.la \
	$(top_builddir)/extensions/ModbusMaster/libUniSet2MBTCPMaster.la \
	$(top_builddir)/extensions/ModbusMaster/libUniSet2RTU.la \
	$(top_builddir)/extensions/ModbusSlave/libUniSet2MBSlave.la \
	$(SIGC_LIBS)

@PACKAGE@_uno_CPPFLAGS = $(SIGC_CFLAGS) -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory \
	-I$(top_builddir)/extensions/UNetUDP \
	-I$(top_builddir)/extensions/ModbusMaster \
	-I$(top_builddir)/extensions/ModbusSlave

# Optional extensions
if !DISABLE_OPCUA
@PACKAGE@_uno_LDADD += $(top_builddir)/extensions/OPCUAServer/libUniSet2OPCUAServer.la \
	$(top_builddir)/extensions/OPCUAExchange/libUniSet2OPCUAExchange.la
@PACKAGE@_uno_CPPFLAGS += -DENABLE_OPCUA -I$(top_builddir)/extensions/OPCUAServer \
	-I$(top_builddir)/extensions/OPCUAExchange
endif

if !DISABLE_IO
@PACKAGE@_uno_LDADD += $(top_builddir)/extensions/IOControl/libUniSet2IOControl.la
@PACKAGE@_uno_CPPFLAGS += -DENABLE_IO -I$(top_builddir)/extensions/IOControl
endif

if !DISABLE_LOGICPROC
@PACKAGE@_uno_LDADD += $(top_builddir)/extensions/LogicProcessor/libUniSet2LProcessor.la
@PACKAGE@_uno_CPPFLAGS += -DENABLE_LOGICPROC -I$(top_builddir)/extensions/LogicProcessor
endif

if !DISABLE_MQTT
@PACKAGE@_uno_LDADD += $(top_builddir)/extensions/MQTTPublisher/libUniSet2MQTTPublisher.la
@PACKAGE@_uno_CPPFLAGS += -DENABLE_MQTT -I$(top_builddir)/extensions/MQTTPublisher $(MQTT_CFLAGS)
endif

if !DISABLE_UWEBSOCKETGATE
@PACKAGE@_uno_LDADD += $(top_builddir)/extensions/UWebSocketGate/libUniSet2UWebSocketGate.la
@PACKAGE@_uno_CPPFLAGS += -DENABLE_UWEBSOCKETGATE -I$(top_builddir)/extensions/UWebSocketGate
endif

if !DISABLE_CLICKHOUSE
@PACKAGE@_uno_LDADD += $(top_builddir)/extensions/Backend-ClickHouse/libUniSet2BackendClickHouse.la
@PACKAGE@_uno_CPPFLAGS += -DENABLE_CLICKHOUSE -I$(top_builddir)/extensions/Backend-ClickHouse $(CLICKHOUSE_CFLAGS)
endif

if !DISABLE_PGSQL
@PACKAGE@_uno_LDADD += $(top_builddir)/extensions/DBServer-PostgreSQL/libUniSet2-pgsql.la $(PGSQL_LIBS)
@PACKAGE@_uno_CPPFLAGS += -DENABLE_PGSQL -I$(top_builddir)/extensions/DBServer-PostgreSQL $(PGSQL_CFLAGS)
endif

include $(top_builddir)/include.mk
