if ENABLE_MQTT
bin_PROGRAMS = @PACKAGE@-mqttpublisher

MQTT_VER=@LIBVER@

lib_LTLIBRARIES = libUniSet2MQTTPublisher.la
libUniSet2MQTTPublisher_la_LDFLAGS  = -version-info $(MQTT_VER)
libUniSet2MQTTPublisher_la_LIBADD		= $(top_builddir)/lib/libUniSet2.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
									$(SIGC_LIBS) $(MQTT_LIBS)
libUniSet2MQTTPublisher_la_CXXFLAGS	= -I$(top_builddir)/extensions/include \
									-I$(top_builddir)/extensions/SharedMemory \
									$(SIGC_CFLAGS) $(MQTT_CFLAGS)
libUniSet2MQTTPublisher_la_SOURCES 	= MQTTPublisher.cc

@PACKAGE@_mqttpublisher_SOURCES 	= main.cc
@PACKAGE@_mqttpublisher_LDADD 	= libUniSet2MQTTPublisher.la $(top_builddir)/lib/libUniSet2.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
									$(SIGC_LIBS) $(MQTT_LIBS)
@PACKAGE@_mqttpublisher_CXXFLAGS	= -I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/SharedMemory \
	$(SIGC_CFLAGS) $(MQTT_CFLAGS)

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions/mqtt

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet2MQTTPublisher.pc

#all-local:
#	ln -sf ../MQTTPublisher/$(devel_include_HEADERS) ../include

include $(top_builddir)/include.mk

endif
