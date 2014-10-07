
#TESTS=$(top_builddir)/tests/tests $(top_builddir)/tests/tests_with_conf.sh

TESTSUITE_AT=testsuite.at
TESTSUITE=testsuite

EXTRA_DIR=package.m4 $(TESTS) $(TESTSUITE) $(TESTSUITE_AT)

testsuite: atconfig package.m4 $(TESTS) $(TESTSUITE_AT)
	autom4te -l autotest $(TESTSUITE_AT) -o $@

clean-local:
	rm -rf *.log atconfig package.m4 $(TESTSUITE)

check-local: atconfig package.m4 $(TESTSUITE) $(TESTS)
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)

package.m4: Makefile
	:;{ \
	  echo '# Signature of the current package.' && \
	  echo 'm4_define([AT_PACKAGE_NAME],      [$(PACKAGE_NAME)])' && \
	  echo 'm4_define([AT_PACKAGE_TARNAME],   [$(PACKAGE_TARNAME)])' && \
	  echo 'm4_define([AT_PACKAGE_VERSION],   [$(PACKAGE_VERSION)])' && \
	  echo 'm4_define([AT_PACKAGE_STRING],    [$(PACKAGE_STRING)])' && \
	  echo 'm4_define([AT_PACKAGE_BUGREPORT], [$(PACKAGE_BUGREPORT)])' && \
	  echo 'm4_define([AT_PACKAGE_URL],       [$(PACKAGE_URL)])'; \
	} > $@-t
	mv $@-t $@

atconfig: $(top_builddir)/config.status
	cd $(top_builddir) && ./config.status testsuite/$@