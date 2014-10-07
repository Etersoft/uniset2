############################################################################
# This file is part of the UniSet library                                  #
############################################################################

check_PROGRAMS = tests tests_with_conf

#umutex sscanf_hexthreadtst dlog

tests_SOURCES = tests.cc passivetimer.cc hourglass.cc delaytimer.cc unixml.cc
tests_LDADD 	= $(top_builddir)/lib/libUniSet2.la
tests_CPPFLAGS	= -I$(top_builddir)/include -I$(includedir)/Catch

tests_with_conf_SOURCES = tests_with_conf.cc conftest.cc ui.cc
tests_with_conf_LDADD   = $(top_builddir)/lib/libUniSet2.la
tests_with_conf_CPPFLAGS = -I$(top_builddir)/include -I$(includedir)/Catch
     
# ui_SOURCES 	= ui.cc
# ui_LDADD 	= $(top_builddir)/lib/libUniSet2.la
# ui_CPPFLAGS	= -I$(top_builddir)/include
# 
# umutex_SOURCES 	= umutex.cc
# umutex_LDADD 	= $(top_builddir)/lib/libUniSet2.la $(COMCPP_LIBS)
# umutex_CPPFLAGS	= -I$(top_builddir)/include $(COMCPP_CFLAGS)
# 
# sscanf_hex_SOURCES 	= sscanf_hex.cc
# 
# threadtst_SOURCES = threadtst.cc
# threadtst_LDADD 	= $(top_builddir)/lib/libUniSet2.la ${SIGC_LIBS} $(COMCPP_LIBS)
# threadtst_CPPFLAGS	= -I$(top_builddir)/include ${SIGC_CFLAGS} $(COMCPP_CFLAGS)
# 
# dlog_SOURCES = dlog.cc
# dlog_LDADD 	= $(top_builddir)/lib/libUniSet2.la ${SIGC_LIBS} $(COMCPP_LIBS)
# dlog_CPPFLAGS = -I$(top_builddir)/include ${SIGC_CFLAGS} $(COMCPP_CFLAGS)
# 

# testdir=$(top_builddir)/tests
#check-local: tests tests_with_conf
#	$(SHELL) $(testdir)/tests 2>/dev/null
#	$(SHELL) $(testdir)/tests_with_conf -- --confile $(testdir)/tests_with_conf.xml --prop-id2 -10 --ulog-no-debug 2>/dev/null

# TESTS_ENVIRONMENT=$(CATCH_TESTS_ENVIRONMENT)
TESTS=tests tests_with_conf.sh

TESTSUITE_AT=testsuite.at
TESTSUITE=testsuite

EXTRA_DIR=package.m4 $(TESTS) $(TESTSUITE) $(TESTSUITE_AT)

testsuite: package.m4 $(TESTS) $(TESTSUITE_AT)
	autom4te -l autotest $(TESTSUITE_AT) -o $@

clean-local:
	rm -rf *.log

check-local: atconfig $(TESTSUITE) $(TESTS)
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
