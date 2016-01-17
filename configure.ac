#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# See doc: http://www.gnu.org/software/hello/manual/autoconf/Generic-Programs.html

# AC_PREREQ(2.59)
AC_INIT([uniset2], [2.3.0], pv@etersoft.ru)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME,AC_PACKAGE_VERSION)

LIBVER=2:3:0
AC_SUBST(LIBVER)

# AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER(uniset-config.h)

dnl Initialize maintainer mode
AM_MAINTAINER_MODE


dnl Checks for programs.
AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AC_LANG_CPLUSPLUS
AC_PROG_INSTALL

AC_PROG_LN_S
AC_PROG_MAKE_SET

dnl Initialize libtool
AC_DISABLE_STATIC
AC_ENABLE_SHARED(yes)
AC_ENABLE_STATIC(no)
AM_PROG_LIBTOOL

ASTYLE_OPT="-A1 -T -C -S -N -L -w -Y -M -f -p --mode=c --lineend=linux --align-reference=type --align-pointer=type --suffix=none --style=ansi"
AC_SUBST(ASTYLE_OPT)

# Checks for libraries.
PKG_CHECK_MODULES(XML, libxml-2.0)
PKG_CHECK_MODULES(OMNI, omniORB4)
PKG_CHECK_MODULES(SIGC, sigc++-2.0)
PKG_CHECK_MODULES(COMCPP, libccgnu2)
PKG_CHECK_MODULES(EV, libev)


#check sqlite support
AC_MSG_CHECKING([sqlite support])
buildsqlite=true
AC_ARG_ENABLE(sqlite, AC_HELP_STRING([--disable-sqlite], [disable SQLite support]),
[ if test $enableval = yes; then buildsqlite=true; else buildsqlite=false; fi],[ buildsqlite=true; ])

if test ${buildsqlite} = true; then
	AC_MSG_RESULT([enabled])
	PKG_CHECK_MODULES(SQLITE3, sqlite3)
else
	AC_MSG_RESULT([disabled])
fi

AM_CONDITIONAL(DISABLE_SQLITE, test ${buildsqlite} = false)

# check xsltproc
AC_CHECK_PROG([XSLTPROC],[xsltproc],[yes])
test "x$XSLTPROC" = "x" && AC_MSG_ERROR([*** xsltproc not found.])

# check IDL compiler
AC_PATH_PROG([IDL], [omniidl])
test "x$IDL" = "x" && AC_MSG_ERROR([*** omniidl not found.])
#UNISET_IDLFLAGS=$(pkg-config omniORB4 --variable=idldir"
#AC_SUBST(UNISET_IDLFLAGS)

#check mysql support
AC_MSG_CHECKING([mysql support])
buildmysql=true
AC_ARG_ENABLE(mysql, AC_HELP_STRING([--disable-mysql], [disable MySQL support]),
[ if test $enableval = yes; then buildmysql=true; else buildmysql=false; fi],[ buildmysql=true; ])

if test ${buildmysql} = true; then
	AC_MSG_RESULT([enabled])
	AC_CHECK_HEADERS([mysql/mysql.h])
	AC_CHECK_LIB([mysqlclient],mysql_init,,exit)
else
	AC_MSG_RESULT([disabled])
fi

AM_CONDITIONAL(DISABLE_MYSQL, test ${buildmysql} = false)

#check pgsql support
AC_MSG_CHECKING([postgresql support])
buildpgsql=true
AC_ARG_ENABLE(pgsql, AC_HELP_STRING([--disable-pgsql], [disable PostgreSQL support]),
[ if test $enableval = yes; then buildpgsql=true; else buildpgsql=false; fi],[ buildpgsql=true; ])

if test ${buildpgsql} = true; then
	AC_MSG_RESULT([enabled])
	PKG_CHECK_MODULES(PGSQL,libpqxx)
	AC_SUBST(PGSQL_LIBS)
	AC_SUBST(PGSQL_CFLAGS)
else
	AC_MSG_RESULT([disabled])
fi

AM_CONDITIONAL(DISABLE_PGSQL, test ${buildpgsql} = false)


#check rrd support
AC_MSG_CHECKING([rrd support])
buildrrd=true
AC_ARG_ENABLE(rrd, AC_HELP_STRING([--disable-rrd], [disable RRD support]),
[ if test $enableval = yes; then buildrrd=true; else buildrrd=false; fi],[ buildrrd=true; ])

if test ${buildrrd} = true; then
	AC_MSG_RESULT([enabled])
	PKG_CHECK_MODULES(RRD,librrd)
	AC_SUBST(RRD_LIBS)
	AC_SUBST(RRD_CFLAGS)
else
	AC_MSG_RESULT([disabled])
fi

AM_CONDITIONAL(DISABLE_RRD, test ${buildrrd} = false)

#check io support
AC_MSG_CHECKING([io support])
buildio=true
AC_ARG_ENABLE(io, AC_HELP_STRING([--disable-io], [disable IO support]),
[ if test $enableval = yes; then buildio=true; else buildio=false; fi],[ buildio=true; ])

if test ${buildio} = true; then
	AC_MSG_RESULT([enabled])

	# check comedilib
	PKG_CHECK_MODULES(COMEDI,comedilib)
	AC_SUBST(COMEDI_LIBS)
	AC_SUBST(COMEDI_CFLAGS)
else
	AC_MSG_RESULT([disabled])
fi

AM_CONDITIONAL(DISABLE_IO, test ${buildio} = false)
AM_CONDITIONAL(ENABLE_IO, test ${buildio} = true)

#check logicproc build
AC_MSG_CHECKING([build logic processor extension])
buildlproc=true
AC_ARG_ENABLE(rrd, AC_HELP_STRING([--disable-logicproc], [disable build LogicProcessor extension]),
[ if test $enableval = yes; then buildlproc=true; else buildlproc=false; fi],[ buildlproc=true; ])
if test ${buildlproc} = true; then
	AC_MSG_RESULT([enabled])
else
	AC_MSG_RESULT([disabled])
fi

AM_CONDITIONAL(DISABLE_LOGICPROC, test ${buildlproc} = false)

#check build extensions
ext=true
AC_MSG_CHECKING([Extensions])
AC_ARG_ENABLE(extensions,AC_HELP_STRING([--disable-extensions], [disable build uniset extensions]),
[ if test $enableval = yes; then ext=true; else ext=false; fi],[ ext=true; ])

if test ${ext} = true; then
	AC_MSG_RESULT([enabled])
else
	AC_MSG_RESULT([disabled])
fi

AM_CONDITIONAL(HAVE_EXTENTIONS, test ${ext} = true)


#check python support
AC_MSG_CHECKING([python support])
buildpython=true
AC_ARG_ENABLE(python, AC_HELP_STRING([--disable-python], [disable Python support]),
[ if test $enableval = yes; then buildpython=true; else buildpython=false; fi],[ buildpython=true; ])

if test ${buildpython} = true; then
	AC_MSG_RESULT([enabled])
	dnl Python
	AM_PATH_PYTHON(,,)
	PKG_CHECK_MODULES(PYTHON,python,,exit)
#	AC_CHECK_PROG(SWIG, swig, yes, exit)
else
	AC_MSG_RESULT([disabled])
fi

AM_CONDITIONAL(DISABLE_PYTHON, test ${buildpython} = false)

#check mqtt support
AC_MSG_CHECKING([mqtt support])
buildmqtt=true
AC_ARG_ENABLE(mqtt, AC_HELP_STRING([--disable-mqtt], [disable MQTT support]),
[ if test $enableval = yes; then buildmqtt=true; else buildmqtt=false; fi],[ buildmqtt=true; ])

if test ${buildmqtt} = true; then
	AC_MSG_RESULT([enabled])
	AC_CHECK_HEADER(mosquittopp.h)
	MQTT_LIBS=-lmosquittopp
	MQTT_CFLAGS=
	AC_SUBST(MQTT_LIBS)
	AC_SUBST(MQTT_CFLAGS)
else
	AC_MSG_RESULT([disabled])
fi

AM_CONDITIONAL(DISABLE_MQTT, test ${buildmqtt} = false)


# check Doxygen
DOXYGEN=""
docs=false
dox=
AC_MSG_CHECKING([Generate docs])
AC_ARG_ENABLE(docs,AC_HELP_STRING([--enable-docs], [enable build docs]),
[ if test $enableval = yes; then docs=true; else docs=false; fi],[ docs=false; ])

if test ${docs} = true; then
	AC_MSG_RESULT([yes])
	AC_PATH_PROG([DOXYGEN], [doxygen])
	test "x$DOXYGEN" = "x" && AC_MSG_WARN([*** Doxygen not found])
else
	DOXYGEN=
	AC_MSG_RESULT([no])
fi
AC_SUBST(DOXYGEN)
AM_CONDITIONAL(HAVE_DOXYGEN, test x$DOXYGEN != x)

if test "x$UNISET_CODEGEN" = x ; then
	UNISET_CODEGEN="sh \$(top_builddir)/Utilities/codegen/uniset2-codegen --path \$(top_builddir)/Utilities/codegen/"
fi
AC_SUBST(UNISET_CODEGEN)
echo "UNISET_CODEGEN: $UNISET_CODEGEN"

UNISET_CFLAGS="-I \$(top_builddir) -I \$(top_builddir)/include"
AC_SUBST(UNISET_CFLAGS)
UNISET_LIBS="\$(top_builddir)/lib/libUniSet2.la"
AC_SUBST(UNISET_LIBS)

UNISET_EXT_CFLAGS="-I \$(top_builddir)/extensions/include"
AC_SUBST(UNISET_EXT_CFLAGS)
UNISET_EXT_LIBS="\$(top_builddir)/extensions/lib/libUniSet2Extensions.la"
AC_SUBST(UNISET_EXT_LIBS)


COV_LIBS=
COV_CFLAGS=
AC_MSG_CHECKING([tests support])
buildtests=true
buildgcov=false
AC_ARG_ENABLE(tests, AC_HELP_STRING([--disable-tests], [disable tests build]),
[ if test $enableval = yes; then buildtests=true; else buildtests=false; fi],[ buildtests=true; ])

if test ${buildtests} = true; then
    AC_MSG_RESULT([yes])
#    AC_LANG_PUSH([C++])
#    AC_CHECK_HEADERS([catch.hpp],,exit)
#    AC_LANG_POP([C++])

	dnl Initialize test suite
	AC_CONFIG_TESTDIR(testsuite)
	AC_CONFIG_TESTDIR(tests)
	AC_CONFIG_TESTDIR(extensions/tests)
	AC_CONFIG_TESTDIR(extensions/LogicProcessor/tests)
	AC_CONFIG_TESTDIR(extensions/ModbusSlave/tests)
	AC_CONFIG_TESTDIR(extensions/ModbusMaster/tests)
	AC_CONFIG_TESTDIR(extensions/UNetUDP/tests)
	AC_CONFIG_TESTDIR(extensions/SharedMemory/tests)

	TESTSUITE_DIR="\$(top_builddir)/testsuite"
	AC_SUBST(TESTSUITE_DIR)

	#check gcov
	AC_MSG_CHECKING([test coverage])
	AC_ARG_ENABLE(coverage, AC_HELP_STRING([--enable-coverage], [enable gcov support]),
	[ if test $enableval = yes; then buildgcov=true; else buildgcov=false; fi],[])
    if test ${buildgcov} = true; then
    	AC_MSG_RESULT([yes])
		COV_LIBS="-lgcov"
		COV_CFLAGS="-fprofile-arcs -ftest-coverage -g -ggdb"
		AC_PATH_PROG([LCOV], [lcov])
		test "x$LCOV" = "x" && AC_MSG_ERROR([*** lcov not found (install lcov or use --disable-coverage)])
		AC_SUBST(LCOV)
		AC_PATH_PROG([LCOV_GENHTML], [genhtml])
		test "x$LCOV_GENHTML" = "x" && AC_MSG_ERROR([*** genhtml not found (install lcov or use --disable-coverage)])
		AC_SUBST(LCOV_GENHTML)
	else
    	AC_MSG_RESULT([no])
	fi

else
    AC_MSG_RESULT([no])
fi

AC_SUBST(COV_LIBS)
AC_SUBST(COV_CFLAGS)

AM_CONDITIONAL(HAVE_COVERAGE, test ${buildgcov} = true)
AM_CONDITIONAL(HAVE_TESTS, test ${buildtests} = true)

# -D_GLIBCXX_USE_NANOSLEEP - for std::this_thread::sleep_for

# -Weffc++ -Wno-unused-variable -Werror -Wctor-dtor-privacy
CXX_EXTRA_FLAGS="-Wnon-virtual-dtor -Woverloaded-virtual -Woverflow -D_GLIBCXX_USE_NANOSLEEP"

# export
LDFLAGS="$LDFLAGS ${OMNI_LIBS} ${XML_LIBS} ${SIGC_LIBS} ${COV_LIBS} ${COMCPP_LIBS}"
# all developer liked options add to autogen.sh, please
CXXFLAGS="$CXXFLAGS -funsigned-char -std=c++11 -g -D_GNU_SOURCE ${OMNI_CFLAGS} ${XML_CFLAGS} ${SIGC_CFLAGS} ${COV_CFLAGS} ${COMCPP_CFLAGS} -I\$(top_builddir)/include $CXX_EXTRA_FLAGS"

AC_SUBST(LDFLAGS)
AC_SUBST(CXXFLAGS)

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h sys/time.h sys/socket.h syslog.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_C_VOLATILE

# Checks for library functions.
#AC_FUNC_CLOSEDIR_VOID
#AC_FUNC_ERROR_AT_LINE
#AC_FUNC_MALLOC
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([atexit getcwd gettimeofday atomic_set])

AC_CONFIG_FILES([Makefile
				 libUniSet2.pc
				 lib/Makefile
				 IDL/Makefile
				 IDL/UniSetTypes/Makefile
				 IDL/Processes/Makefile
				 src/Communications/Makefile
				 src/Communications/Modbus/Makefile
				 src/Communications/TCP/Makefile
				 src/Interfaces/Makefile
				 src/ObjectRepository/Makefile
				 src/Processes/Makefile
				 src/Services/Makefile
				 src/Timers/Makefile
				 src/Various/Makefile
				 src/Log/Makefile
				 src/Makefile
				 include/Makefile
				 include/modbus/Makefile
				 tests/Makefile
				 tests/UniXmlTest/Makefile
				 docs/Makefile
				 docs/UniSetDox.cfg
				 docs/UniSetDoxDevel.cfg
				 Utilities/Makefile
				 Utilities/scripts/Makefile
				 Utilities/Admin/Makefile
				 Utilities/NullController/Makefile
				 Utilities/SViewer-text/Makefile
				 Utilities/SMonit/Makefile
				 Utilities/MBTester/Makefile
				 Utilities/SImitator/Makefile
				 Utilities/codegen/Makefile
				 Utilities/codegen/uniset2-codegen
				 Utilities/codegen/tests/Makefile
				 Utilities/ULog/Makefile
				 extensions/Makefile
				 extensions/libUniSet2Extensions.pc
				 extensions/lib/Makefile
				 extensions/include/Makefile
				 extensions/DBServer-MySQL/Makefile
				 extensions/DBServer-MySQL/libUniSet2MySQL.pc
				 extensions/DBServer-SQLite/Makefile
				 extensions/DBServer-SQLite/libUniSet2SQLite.pc
				 extensions/DBServer-PostgreSQL/Makefile
				 extensions/DBServer-PostgreSQL/libUniSet2PostgreSQL.pc
				 extensions/IOControl/Makefile
				 extensions/IOControl/libUniSet2IOControl.pc
				 extensions/ModbusMaster/Makefile
				 extensions/ModbusMaster/libUniSet2RTU.pc
				 extensions/ModbusMaster/libUniSet2MBTCPMaster.pc
				 extensions/ModbusMaster/tests/Makefile
				 extensions/ModbusSlave/Makefile
				 extensions/ModbusSlave/libUniSet2MBSlave.pc
				 extensions/ModbusSlave/tests/Makefile
				 extensions/ModbusSlave/epoll-test/Makefile
				 extensions/LogicProcessor/Makefile
				 extensions/LogicProcessor/libUniSet2LogicProcessor.pc
				 extensions/LogicProcessor/tests/Makefile
				 extensions/SMViewer/Makefile
				 extensions/UniNetwork/Makefile
				 extensions/UniNetwork/libUniSet2Network.pc
				 extensions/UNetUDP/Makefile
				 extensions/UNetUDP/libUniSet2UNetUDP.pc
				 extensions/UNetUDP/tests/Makefile
				 extensions/SharedMemory/Makefile
				 extensions/SharedMemory/tests/Makefile
				 extensions/SharedMemory/libUniSet2SharedMemory.pc
				 extensions/SharedMemoryPlus/Makefile
				 extensions/RRDServer/Makefile
				 extensions/RRDServer/libUniSet2RRDServer.pc
				 extensions/MQTTPublisher/Makefile
				 extensions/MQTTPublisher/libUniSet2MQTTPublisher.pc
				 extensions/tests/Makefile
				 extensions/tests/SMemoryTest/Makefile
				 testsuite/Makefile
				 python/lib/Makefile
				 python/lib/pyUniSet/Makefile
				 python/Makefile])
				 

AC_OUTPUT

#				 extensions/SMDBServer/Makefile
#				 extensions/SMDBServer/libUniSet2SMDBServer.pc

