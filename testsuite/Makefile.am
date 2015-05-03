if HAVE_TESTS

# Common rules fot uniset testsuite
include $(top_builddir)/testsuite/testsuite-common.mk

check-local: atconfig package.m4 $(TESTSUITE)
	$(SHELL) $(TESTSUITE) $(TESTSUITEFLAGS)
	
clean-local:
	test ! -f '$(TESTSUITE)'|| $(SHELL) '$(TESTSUITE)' --clean
	rm -rf $(CLEANFILES)

include $(top_builddir)/include.mk

endif
