# Common rules fot uniset testsuite

TESTSUITE_AT=testsuite.at
TESTSUITE=testsuite

EXTRA_DIR=atconfig package.m4 $(TESTSUITE) $(TESTSUITE_AT)

testsuite: atconfig package.m4 $(TESTSUITE_AT)
	autom4te -l autotest $(TESTSUITE_AT) -o $@

CLEANFILES=*.log package.m4 $(TESTSUITE)

package.m4: Makefile
	@:;{ \
	  echo '# Signature of the current package.' && \
	  echo 'm4_define([AT_PACKAGE_NAME],      [$(PACKAGE_NAME)])' && \
	  echo 'm4_define([AT_PACKAGE_TARNAME],   [$(PACKAGE_TARNAME)])' && \
	  echo 'm4_define([AT_PACKAGE_VERSION],   [$(PACKAGE_VERSION)])' && \
	  echo 'm4_define([AT_PACKAGE_STRING],    [$(PACKAGE_STRING)])' && \
	  echo 'm4_define([AT_PACKAGE_BUGREPORT], [$(PACKAGE_BUGREPORT)])' && \
	  echo 'm4_define([AT_PACKAGE_URL],       [$(PACKAGE_URL)])' && \
	  echo 'm4_define([AT_TEST_LAUNCH],       [$(TESTSUITE_DIR)/at-test-launch.sh)])' && \
	  echo 'm4_define([AT_TOP_BUILDDIR],       [$(top_builddir)))])' && \
	  echo 'm4_define([AT_TESTSUITE_DIR], [$(TESTSUITE_DIR))])'; \
	} > $@-t
	@mv $@-t $@

atconfig: $(top_builddir)/config.status
	cd $(top_builddir) && ./config.status $(abs_srcdir)/$@
