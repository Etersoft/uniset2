bin_PROGRAMS 		= @PACKAGE@-smemory-plus

@PACKAGE@_smemory_plus_LDADD 		= $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/UNetUDP/libUniSet2UNetUDP.la \
	$(top_builddir)/extensions/ModbusMaster/libUniSet2MBTCPMaster.la \
	$(top_builddir)/extensions/ModbusMaster/libUniSet2RTU.la \
	$(top_builddir)/extensions/ModbusSlave/libUniSet2MBSlave.la \
	$(top_builddir)/extensions/IOControl/libUniSet2IOControl.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(SIGC_LIBS) $(COMCPP_LIBS)

@PACKAGE@_smemory_plus_CXXFLAGS	= -I$(top_builddir)/extensions/CanNetwork \
	-I$(top_builddir)/extensions/UNetUDP \
	-I$(top_builddir)/extensions/ModbusMaster \
	-I$(top_builddir)/extensions/ModbusSlave \
	-I$(top_builddir)/extensions/IOControl \
	-I$(top_builddir)/extensions/SharedMemory \
	-I$(top_builddir)/extensions/include \
	$(SIGC_CFLAGS) $(COMCPP_CFLAGS)

@PACKAGE@_smemory_plus_SOURCES = smemory-plus.cc
