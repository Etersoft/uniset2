#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# AC_PREREQ(2.59)
AC_INIT([uniset], [0.97], pv@etersoft.ru)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME,AC_PACKAGE_VERSION)

# AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER(config.h)

dnl Initialize maintainer mode
AM_MAINTAINER_MODE

dnl Checks for programs.
AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AC_LANG_CPLUSPLUS dnl switch to C++
AC_PROG_INSTALL

AC_PROG_LN_S
AC_PROG_MAKE_SET


dnl Initialize libtool
AC_DISABLE_STATIC
AC_ENABLE_SHARED(yes)
AC_ENABLE_STATIC(no)
AM_PROG_LIBTOOL

LIBVER=0:9:0
AC_SUBST(LIBVER)

# Checks for libraries.
PKG_CHECK_MODULES(XML, libxml-2.0)
PKG_CHECK_MODULES(OMNI, omniORB4)
#OMNI_CFLAGS=-D__OMNIORB4
#OMNI_LIBS=-lomniORB4 

PKG_CHECK_MODULES(SIGC, sigc++-2.0)
AC_SUBST(SIGC_CFLAGS)
AC_SUBST(SIGC_LIBS)

PKG_CHECK_MODULES(COMCPP, libccgnu2)
AC_SUBST(COMCPP_CFLAGS)
AC_SUBST(COMCPP_LIBS)

# export
LDFLAGS="${OMNI_LIBS} ${XML_LIBS}"
CXXFLAGS="-pedantic -Wall -funsigned-char -g -D_GNU_SOURCE ${OMNI_CFLAGS} ${XML_CFLAGS} -I\$(top_builddir)/include"

AC_SUBST(LDFLAGS)
AC_SUBST(CXXFLAGS)

# check IDL compiler
AC_CHECK_PROG([IDL], [omniidl], [omniidl], , , AC_MSG_ERROR([*** omniidl not found.]))
#UNISET_IDLFLAGS=$(pkg-config omniORB4 --variable=idldir"
AC_SUBST(IDL)
#AC_SUBST(UNISET_IDLFLAGS)

#check mysql support
AC_MSG_CHECKING([mysql support])
dmysql=false
AC_ARG_ENABLE(mysql, AC_HELP_STRING([--disable-mysql], [disable MySQL support]),
[ if test $enableval = yes; then dmysql=false; else dmysql=true; fi],[ dmysql=false; ])

if test ${dmysql} = false; then
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
fi

AM_CONDITIONAL(DISABLE_MYSQL, test ${dmysql} = true)

#check build extensions
ext=true
AC_MSG_CHECKING([Extensions])
AC_ARG_ENABLE(extensions,AC_HELP_STRING([--disable-extensions], [disable build uniset extensions]),
[ if test $enableval = yes; then ext=true; else ext=false; fi],[ ext=true; ])

if test ${ext} = true; then
	AC_MSG_RESULT([enabled])

	# check comedilib
	PKG_CHECK_MODULES(COMEDI,comedilib)
	AC_SUBST(COMEDI_LIBS)
	AC_SUBST(COMEDI_CFLAGS)
else
	AC_MSG_RESULT([disabled])
fi

AM_CONDITIONAL(HAVE_EXTENTIONS, test ${ext} = true)

# check Doxygen
DOXYGEN=""
doc=true
dox=
AC_MSG_CHECKING([Generate docs])
AC_ARG_ENABLE(docs,AC_HELP_STRING([--disable-docs], [disable build docs]),
[ if test $enableval = yes; then doc=true; else doc=false; fi],[ doc=true; ])

if test ${doc} = true; then
	AC_MSG_RESULT([yes])
	AC_CHECK_PROG(dox,doxygen,[yes])
	if test "x$dox" != x ; then
		DOXYGEN="doxygen"
	else
		AC_MSG_WARN([*** Doxygen not found])
	fi
else
	DOXYGEN=
	AC_MSG_RESULT([no])
fi
AC_SUBST(DOXYGEN)
AM_CONDITIONAL(HAVE_DOXYGEN, test x$DOXYGEN != x)

if test "x$UNISET_CODEGEN" = x ; then
	UNISET_CODEGEN="sh \$(top_builddir)/Utilities/codegen/uniset-codegen --path \$(top_builddir)/Utilities/codegen/"
fi
AC_SUBST(UNISET_CODEGEN)
echo "UNISET_CODEGEN: $UNISET_CODEGEN"

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
				 libUniSet.pc
				 lib/Makefile
				 IDL/Makefile
				 IDL/UniSetTypes/Makefile
				 IDL/Processes/Makefile
				 IDL/Services/Makefile
				 src/Communications/Makefile
				 src/Communications/Modbus/Makefile
				 src/IOs/Makefile
				 src/Interfaces/Makefile
				 src/ObjectRepository/Makefile
				 src/Processes/Makefile
				 src/Services/Makefile
				 src/Threads/Makefile
				 src/Timers/Makefile
				 src/Various/Makefile
				 src/Makefile
				 include/Makefile
				 include/modbus/Makefile
				 include/IOs/Makefile
				 tests/Makefile
				 docs/Makefile
				 docs/UniSetDox.cfg
				 docs/UniSetDoxDevel.cfg
				 Utilities/Makefile
				 Utilities/scripts/Makefile
				 Utilities/Admin/Makefile
				 Utilities/NullController/Makefile
				 Utilities/SViewer-text/Makefile
				 Utilities/SMonit/Makefile
				 Utilities/InfoServer/Makefile
				 Utilities/MBTester/Makefile
				 Utilities/DBServer-MySQL/Makefile
				 Utilities/SImitator/Makefile
				 Utilities/JrnTests/Makefile
				 Utilities/UniXmlTest/Makefile
				 Utilities/codegen/Makefile
				 Utilities/codegen/uniset-codegen
				 Utilities/codegen/tests/Makefile
				 extensions/Makefile
				 extensions/libUniSetExtensions.pc
				 extensions/lib/Makefile
				 extensions/include/Makefile
				 extensions/IOControl/Makefile
				 extensions/IOControl/libUniSetIOControl.pc
				 extensions/RTUExchange/Makefile
				 extensions/RTUExchange/libUniSetRTU.pc
				 extensions/UDPExchange/Makefile
				 extensions/UDPExchange/libUniSetUDP.pc
				 extensions/ModbusSlave/Makefile
				 extensions/ModbusSlave/libUniSetMBSlave.pc
				 extensions/MBTCPMaster/Makefile
				 extensions/MBTCPMaster/libUniSetMBTCPMaster.pc
				 extensions/LogicProcessor/Makefile
				 extensions/LogicProcessor/libUniSetLogicProcessor.pc
				 extensions/SMViewer/Makefile
				 extensions/UniNetwork/Makefile
				 extensions/UniNetwork/libUniSetNetwork.pc
				 extensions/SharedMemory/Makefile
				 extensions/SharedMemory/libUniSetSharedMemory.pc
				 extensions/SharedMemoryPlus/Makefile])
				 
AC_OUTPUT

#				 extensions/SMDBServer/Makefile
#				 extensions/SMDBServer/libUniSetSMDBServer.pc
