bin_PROGRAMS 		= @PACKAGE@-smemory-plus

@PACKAGE@_smemory_plus_LDADD 		= $(top_builddir)/lib/libUniSet.la \
	$(top_builddir)/extensions/UNetUDP/libUniSetUNetUDP.la \
	$(top_builddir)/extensions/ModbusMaster/libUniSetMBTCPMaster.la \
	$(top_builddir)/extensions/ModbusMaster/libUniSetRTU.la \
	$(top_builddir)/extensions/ModbusSlave/libUniSetMBSlave.la \
	$(top_builddir)/extensions/IOControl/libUniSetIOControl.la \
	$(top_builddir)/extensions/SharedMemory/libUniSetSharedMemory.la \
	$(top_builddir)/extensions/lib/libUniSetExtensions.la \
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
