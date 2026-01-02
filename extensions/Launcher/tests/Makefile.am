############################################################################
# This file is part of the UniSet library                                 #
############################################################################

if HAVE_TESTS

noinst_PROGRAMS = tests

tests_SOURCES = tests.cc \
	test_dependency_resolver.cc \
	test_config_loader.cc \
	test_process_manager.cc \
	test_http_handler.cc \
	test_process_template.cc \
	test_process_info.cc

tests_LDADD = $(top_builddir)/lib/libUniSet2.la \
	$(top_builddir)/extensions/lib/libUniSet2Extensions.la \
	$(top_builddir)/extensions/Launcher/libUniSet2Launcher.la \
	$(SIGC_LIBS) $(POCO_LIBS) -lpthread

tests_CPPFLAGS = -I$(top_builddir)/include \
	-I$(top_builddir)/extensions/include \
	-I$(top_builddir)/extensions/Launcher \
	$(SIGC_CFLAGS) $(POCO_CFLAGS)

$(top_builddir)/extensions/lib/libUniSet2Extensions.la:
	cd $(top_builddir)/extensions/lib/ && $(MAKE)

$(top_builddir)/extensions/Launcher/libUniSet2Launcher.la:
	cd $(top_builddir)/extensions/Launcher/ && $(MAKE)

check-local: tests
	./tests

clean-local:
	rm -rf $(CLEANFILES)

include $(top_builddir)/include.mk

endif
