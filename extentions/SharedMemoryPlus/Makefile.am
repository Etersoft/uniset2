bin_PROGRAMS 		= @PACKAGE@-smemory-plus

@PACKAGE@_smemory_plus_LDADD 		= $(top_builddir)/lib/libUniSet.la \
	$(top_builddir)/extentions/RTUExchange/libUniSetRTU.la \
	$(top_builddir)/extentions/ModbusSlave/libUniSetMBSlave.la \
	$(top_builddir)/extentions/IOControl/libUniSetIOControl.la \
	$(top_builddir)/extentions/SharedMemory/libUniSetSharedMemory.la \
	$(top_builddir)/extentions/lib/libUniSetExtentions.la \
	$(SIGC_LIBS)

@PACKAGE@_smemory_plus_CXXFLAGS	= -I$(top_builddir)/extentions/CanNetwork \
	-I$(top_builddir)/extentions/RTUExchange \
	-I$(top_builddir)/extentions/ModbusSlave \
	-I$(top_builddir)/extentions/IOControl \
	-I$(top_builddir)/extentions/SharedMemory \
	-I$(top_builddir)/extentions/include \
	$(SIGC_CFLAGS)

@PACKAGE@_smemory_plus_SOURCES = smemory-plus.cc
