COVERAGE_REPORT_DIR=$(abs_srcdir)/coverage-report
COVERAGE_DEFAULT_DIRS=$(abs_srcdir) $(top_builddir)/src $(top_builddir)/extensions $(top_builddir)/tests

cov:
	if test -z "${COVERAGE_DIRS}"; then echo "UNDEFINED COVERAGE_DIRS="; exit -1; fi
	rm -rf $(abs_srcdir)/.coverage.*
	$(LCOV) -d "${COVERAGE_DIRS}" --zerocounters
	make clean
	test -f /usr/bin/jmake && jmake || make
	make -i check
	if test -z "${COVERAGE_DIRS}"; then echo "UNDEFINED COVERAGE_DIRS="; exit -1; fi
	rm -rf $(abs_srcdir)/.coverage.*
	for d in $(COVERAGE_DIRS); do \
		TNAME=`mktemp .coverage.info.XXXXXXX`; \
		$(LCOV) --capture --directory $$d --output-file $(abs_srcdir)/$$TNAME; \
		if ! test -s $$TNAME; then rm -f $$TNAME; fi; \
	done
	COV_FILES=`ls .coverage.info.*`; COV_CMD=""; \
	for d in $$COV_FILES; do \
		COV_CMD="$$COV_CMD -a $$d"; \
	done; \
	$(LCOV) -d $(abs_srcdir) $$COV_CMD -o $(abs_srcdir)/.coverage.all
	$(LCOV) -r $(abs_srcdir)/.coverage.all '/usr/include/catch.hpp' '/usr/include/*.h' '/usr/include/c++/*' \
	'/usr/include/cc++/*' '/usr/include/omniORB4/*' '/usr/include/sigc++-2.0/sigc++/*' '*.hh' -o $(abs_srcdir)/.coverage.total
	$(LCOV_GENHTML) $(abs_srcdir)/.coverage.total --output-directory $(COVERAGE_REPORT_DIR)
	rm -rf $(abs_srcdir)/.coverage.*

covrep:
	if test -z "${COVERAGE_DIRS}"; then echo "UNDEFINED COVERAGE_DIRS="; exit -1; fi
	rm -rf $(abs_srcdir)/.coverage.*
	for d in $(COVERAGE_DIRS); do \
		TNAME=`mktemp .coverage.info.XXXXXXX`; \
		$(LCOV) --capture --directory $$d --output-file $(abs_srcdir)/$$TNAME; \
		if ! test -s $$TNAME; then rm -f $$TNAME; fi; \
	done
	COV_FILES=`ls .coverage.info.*`; COV_CMD=""; \
	for d in $$COV_FILES; do \
		COV_CMD="$$COV_CMD -a $$d"; \
	done; \
	$(LCOV) -d $(abs_srcdir) $$COV_CMD -o $(abs_srcdir)/.coverage.all
	$(LCOV) -r $(abs_srcdir)/.coverage.all '/usr/include/catch.hpp' '/usr/include/*.h' '/usr/include/c++/*' \
	'/usr/include/cc++/*' '/usr/include/omniORB4/*' '/usr/include/sigc++-2.0/sigc++/*' '*.hh' -o $(abs_srcdir)/.coverage.total
	$(LCOV_GENHTML) $(abs_srcdir)/.coverage.total --output-directory $(COVERAGE_REPORT_DIR)
	rm -rf $(abs_srcdir)/.coverage.*
