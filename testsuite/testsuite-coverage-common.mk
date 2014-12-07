COVERAGE_REPORT_DIR=$(abs_srcdir)/coverage-report

cov: all
	$(LCOV) --directory $(abs_srcdir) --zerocounters
	$(LCOV) -c -i -d $(abs_srcdir) -o $(abs_srcdir)/.coverage.base
	make check
	$(LCOV) -c -d $(abs_srcdir) -o $(abs_srcdir)/.coverage.run
	$(LCOV) -d $(abs_srcdir) -a $(abs_srcdir)/.coverage.base -a $(abs_srcdir)/.coverage.run -o $(abs_srcdir)/.coverage.total
	$(LCOV) -r $(abs_srcdir)/.coverage.total '/usr/include/c++/*' '/usr/include/cc++/*' '/usr/include/omniORB4/*' '/usr/include/sigc++-2.0/sigc++/*' -d $(abs_srcdir) -o $(abs_srcdir)/.coverage.total
	$(LCOV_GENHTML) --no-branch-coverage -o $(COVERAGE_REPORT_DIR) $(abs_srcdir)/.coverage.total
	rm -f $(abs_srcdir)/.coverage.base $(abs_srcdir)/.coverage.run $(abs_srcdir)/.coverage.total
