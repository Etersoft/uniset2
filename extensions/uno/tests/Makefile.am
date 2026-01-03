############################################################################
# This file is part of the UniSet library                                 #
############################################################################

if HAVE_TESTS

noinst_PROGRAMS = tests

tests_SOURCES = tests.cc \
	test_unisetuno.cc \
	../UniSetUno.cc

tests_LDADD = $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(top_builddir)/extensions/UNetUDP/libUniSet2UNetUDP.la \
	$(top_builddir)/extensions/ModbusMaster/libUniSet2MBTCPMaster.la \
	$(top_builddir)/extensions/ModbusMaster/libUniSet2RTU.la \
	$(top_builddir)/extensions/ModbusSlave/libUniSet2MBSlave.la \
	$(SIGC_LIBS) $(POCO_LIBS) -lpthread

tests_CPPFLAGS = -I$(top_builddir)/include \
	-I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/uno \
	-I$(top_builddir)/extensions/SharedMemory \
	-I$(top_builddir)/extensions/UNetUDP \
	-I$(top_builddir)/extensions/ModbusMaster \
	-I$(top_builddir)/extensions/ModbusSlave \
	$(SIGC_CFLAGS) $(POCO_CFLAGS)

# Optional extensions
if !DISABLE_OPCUA
tests_LDADD += $(top_builddir)/extensions/OPCUAServer/libUniSet2OPCUAServer.la \
	$(top_builddir)/extensions/OPCUAExchange/libUniSet2OPCUAExchange.la
tests_CPPFLAGS += -DENABLE_OPCUA -I$(top_builddir)/extensions/OPCUAServer \
	-I$(top_builddir)/extensions/OPCUAExchange
endif

if !DISABLE_IO
tests_LDADD += $(top_builddir)/extensions/IOControl/libUniSet2IOControl.la
tests_CPPFLAGS += -DENABLE_IO -I$(top_builddir)/extensions/IOControl
endif

if !DISABLE_LOGICPROC
tests_LDADD += $(top_builddir)/extensions/LogicProcessor/libUniSet2LProcessor.la
tests_CPPFLAGS += -DENABLE_LOGICPROC -I$(top_builddir)/extensions/LogicProcessor
endif

if !DISABLE_MQTT
tests_LDADD += $(top_builddir)/extensions/MQTTPublisher/libUniSet2MQTTPublisher.la
tests_CPPFLAGS += -DENABLE_MQTT -I$(top_builddir)/extensions/MQTTPublisher $(MQTT_CFLAGS)
endif

if !DISABLE_UWEBSOCKETGATE
tests_LDADD += $(top_builddir)/extensions/UWebSocketGate/libUniSet2UWebSocketGate.la
tests_CPPFLAGS += -DENABLE_UWEBSOCKETGATE -I$(top_builddir)/extensions/UWebSocketGate
endif

if !DISABLE_CLICKHOUSE
tests_LDADD += $(top_builddir)/extensions/Backend-ClickHouse/libUniSet2BackendClickHouse.la
tests_CPPFLAGS += -DENABLE_CLICKHOUSE -I$(top_builddir)/extensions/Backend-ClickHouse $(CLICKHOUSE_CFLAGS)
endif

if !DISABLE_PGSQL
tests_LDADD += $(top_builddir)/extensions/DBServer-PostgreSQL/libUniSet2-pgsql.la $(PGSQL_LIBS)
tests_CPPFLAGS += -DENABLE_PGSQL -I$(top_builddir)/extensions/DBServer-PostgreSQL $(PGSQL_CFLAGS)
endif

check-local: tests
	./tests

clean-local:
	rm -rf $(CLEANFILES)

include $(top_builddir)/include.mk

endif
