if ENABLE_CLICKHOUSE
bin_PROGRAMS = @PACKAGE@-backend-clickhouse @PACKAGE@-clickhouse-helper

CLICKHOUSE_VER=@LIBVER@

lib_LTLIBRARIES = libUniSet2BackendClickHouse.la
libUniSet2BackendClickHouse_la_LDFLAGS  = -version-info $(CLICKHOUSE_VER)
libUniSet2BackendClickHouse_la_LIBADD		= $(top_builddir)/lib/libUniSet2.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
									$(SIGC_LIBS) $(CLICKHOUSE_LIBS)
libUniSet2BackendClickHouse_la_CXXFLAGS	= -Wno-switch -I$(top_builddir)/extensions/include \
									-I$(top_builddir)/extensions/SharedMemory \
									$(SIGC_CFLAGS) $(CLICKHOUSE_CFLAGS)
libUniSet2BackendClickHouse_la_SOURCES 	= BackendClickHouse.cc ClickHouseInterface.cc ClickHouseTagsConfig.cc

@PACKAGE@_backend_clickhouse_SOURCES 	= main.cc
@PACKAGE@_backend_clickhouse_LDADD 	= libUniSet2BackendClickHouse.la $(top_builddir)/lib/libUniSet2.la \
									$(top_builddir)/extensions/SharedMemory/libUniSet2SharedMemory.la \
									$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
									$(SIGC_LIBS) $(CLICKHOUSE_LIBS)
@PACKAGE@_backend_clickhouse_CXXFLAGS	= -I$(top_builddir)/extensions/include \
									-I$(top_builddir)/extensions/SharedMemory \
									$(SIGC_CFLAGS) $(CLICKHOUSE_CFLAGS)

@PACKAGE@_clickhouse_helper_LDADD   = libUniSet2BackendClickHouse.la $(top_builddir)/lib/libUniSet2.la $(CLICKHOUSE_LIBS)
@PACKAGE@_clickhouse_helper_CXXFLAGS = $(CLICKHOUSE_CFLAGS)
@PACKAGE@_clickhouse_helper_SOURCES = helper.cc

noinst_PROGRAMS = clickhouse-test
clickhouse_test_LDADD   = libUniSet2BackendClickHouse.la $(top_builddir)/lib/libUniSet2.la $(CLICKHOUSE_LIBS)
clickhouse_test_CXXFLAGS = $(CLICKHOUSE_CFLAGS)
clickhouse_test_SOURCES = test.cc

bin_SCRIPTS = @PACKAGE@-clickhouse-admin

#xsldir = $(datadir)/@PACKAGE@/xslt
#xsl_DATA = xslt/*.xsl

all-local:
	chmod 'a+x' @PACKAGE@-clickhouse-admin

testdata:
	$(CURDIR)/@PACKAGE@-clickhouse-helper --confile test.xml --generate-dict-objects $(CURDIR)/testsuite/data/dict-objects.csv
	$(CURDIR)/@PACKAGE@-clickhouse-helper --confile test.xml --generate-dict-sensors $(CURDIR)/testsuite/data/dict-sensors.csv
	$(CURDIR)/@PACKAGE@-clickhouse-helper --confile test.xml --generate-dict-nodes $(CURDIR)/testsuite/data/dict-nodes.csv

# install
devel_include_HEADERS = *.h
devel_includedir = $(pkgincludedir)/extensions

pkgconfigdir 	= $(libdir)/pkgconfig
pkgconfig_DATA 	= libUniSet2BackendClickHouse.pc

ddir =  $(datadir)/@PACKAGE@/clickhouse
d_DATA = testsuite/docker-entrypoint-initdb.d/*.sql testsuite/clickhouse_uniset_dictionary.xml

include $(top_builddir)/include.mk

endif
