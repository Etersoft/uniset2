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
	test_process_info.cc \
	test_omninames_manager.cc

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

# Shell-level regression tests. Each script returns 0 on success, non-zero on
# failure. Run from the tests directory so the scripts find the daemons and
# XML configs alongside them.
SHELL_TESTS = \
	test-launcher-test-env.sh \
	test-signal-shutdown.sh \
	test-signal-shutdown-http.sh \
	test-signal-shutdown-unready.sh \
	test-signal-shutdown-slowstart.sh \
	test-exit-code.sh \
	test-max-restarts.sh \
	test-oneshot-timeout.sh \
	test-oneshot-output.sh \
	test-cli-precedence.sh \
	test-passthrough-args.sh

# Files needed at run-time by the shell tests (helper daemons + XML configs).
# Listed in EXTRA_DIST so `make dist` packs them into the tarball.
EXTRA_DIST = $(SHELL_TESTS) \
	launcher-test-env.sh \
	test-signal-shutdown.xml \
	test-signal-shutdown-unready.xml \
	test-signal-shutdown-slowstart.xml \
	test-max-restarts.xml \
	test-oneshot-timeout.xml \
	test-oneshot-output.xml \
	test-passthrough-args.xml \
	crash-daemon.sh \
	slowstart-daemon.sh \
	unready-daemon.sh \
	runtime-crash-daemon.sh \
	hanging-oneshot.sh \
	big-output-oneshot.sh \
	echo-args-daemon.sh

check-local: tests
	./tests
	@for t in $(SHELL_TESTS); do \
	    echo "=== Running $$t ==="; \
	    if ! ./$$t; then \
	        echo "FAIL: $$t"; \
	        exit 1; \
	    fi; \
	done
	@echo "All Launcher shell tests passed."

clean-local:
	rm -rf $(CLEANFILES)

include $(top_builddir)/include.mk

endif
