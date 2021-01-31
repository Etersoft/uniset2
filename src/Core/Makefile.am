noinst_LTLIBRARIES = libUCore.la
libUCore_la_LIBADD = $(top_builddir)/contrib/cityhash102/libCityHash102.la $(top_builddir)/contrib/libfarmhash/libFarmHash.la
libUCore_la_CPPFLAGS = -I$(top_builddir)/contrib/cityhash102/include -I$(top_builddir)/contrib/libfarmhash
libUCore_la_SOURCES = UniSetTypes_iSK.cc UniSetObject_iSK.cc UniSetTypes.cc \
	UniSetManager_iSK.cc UniSetObject.cc UniSetManager.cc UniSetActivator.cc \
	Configuration.cc MessageType.cc UInterface.cc

include $(top_builddir)/include.mk
