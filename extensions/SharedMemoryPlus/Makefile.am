if ENABLE_IO
IO_LIBS=$(top_builddir)/extensions/IOControl/libUniSet2IOControl.la
IO_CFLAGS=-I$(top_builddir)/extensions/IOControl -DUNISET_ENABLE_IO
else
IO_LIBS=
IO_CFLAGS=
endif

bin_PROGRAMS 		= @PACKAGE@-smemory-plus

@PACKAGE@_smemory_plus_LDADD 		= $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/UNetUDP/libUniSet2UNetUDP.la \
	$(top_builddir)/extensions/ModbusMaster/libUniSet2MBTCPMaster.la \
	$(top_builddir)/extensions/ModbusMaster/libUniSet2RTU.la \
	$(top_builddir)/extensions/ModbusSlave/libUniSet2MBSlave.la \
	$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(IO_LIBS) \
	$(SIGC_LIBS) $(COMCPP_LIBS)

@PACKAGE@_smemory_plus_CXXFLAGS	= \
	-I$(top_builddir)/extensions/UNetUDP \
	-I$(top_builddir)/extensions/ModbusMaster \
	-I$(top_builddir)/extensions/ModbusSlave \
	-I$(top_builddir)/extensions/SharedMemory \
	-I$(top_builddir)/extensions/include \
	$(IO_CFLAGS) \
	$(SIGC_CFLAGS) $(COMCPP_CFLAGS)

@PACKAGE@_smemory_plus_SOURCES = smemory-plus.cc

