%def_disable doc
%define oname uniset

Name: libuniset
Version: 0.97
Release: eter6
Summary: UniSet - library for building distributed industrial control systems
License: GPL
Group: Development/C++
Url: http://sourceforge.net/uniset

Packager: Pavel Vainerman <pv@altlinux.ru>

Source: /var/ftp/pvt/Etersoft/Ourside/unstable/sources/tarball/%name-%version.tar

# manually removed: glibc-devel-static
# Automatically added by buildreq on Tue Jul 14 2009
BuildRequires: glibc-devel libMySQL-devel libcomedi-devel libcommoncpp2-devel libomniORB-devel libsigc++2.0-devel python-modules xsltproc

%set_verify_elf_method textrel=strict,rpath=strict,unresolved=strict

%description
The UniSet library intended for building distributed industrial control systems

%package devel
Group: Development/C
Summary: Libraries needed to develop for UniSet
Requires: %name = %version-%release

%description devel
Libraries needed to develop for UniSet.

%package mysql-dbserver
Group: Development/Databases
Summary: MySQL-dbserver implementatioin for UniSet
Requires: %name = %version-%release
Provides: %oname-mysql-dbserver
Obsoletes: %oname-mysql-dbserver

%description mysql-dbserver
MySQL dbserver for %name

%package utils
Summary: UniSet utilities
Group: Development/Tools
Requires: %name = %version-%release
Provides: %oname-utils
Obsoletes: %oname-utils

%description utils
UniSet utilities

%package doc
Group: Development/C
Summary: Documentations for developing with UniSet
Requires: lib%name = %version-%release

%description doc
Documentations for developing with UniSet

%package extensions
Group: Development/Databases
Summary: libUniSet extensions
Requires: %name = %version-%release
Provides: %oname-extentions
Obsoletes: %oname-extentions
Provides: %name-extentions
Obsoletes: %name-extentions

%description extensions
Extensions for libuniset

%package extensions-devel
Group: Development/Databases
Summary: Libraries needed to develop for uniset extensions
Requires: %name-extensions = %version-%release
Provides: %name-extentions-devel
Obsoletes: %name-extentions-devel

%description extensions-devel
Libraries needed to develop for uniset extensions

%prep
%setup -q

%build
%autoreconf
%if_enabled doc
%configure
%else
%configure --disable-docs --disable-static
%endif

%make

%install
%makeinstall_std
rm -f %buildroot%_libdir/*.la

%files utils
%_bindir/%oname-admin
%_bindir/%oname-infoserver
%_bindir/%oname-mb*
%_bindir/%oname-nullController
%_bindir/%oname-sviewer-text
%_bindir/%oname-smonit
%_bindir/%oname-start*
%_bindir/%oname-stop*
%_bindir/%oname-func*
%_bindir/%oname-codegen
%dir %_datadir/%oname/
%dir %_datadir/%oname/xslt/
%_datadir/%oname/xslt/*.xsl

%files
%_libdir/libUniSet.so.*

%files devel
%dir %_includedir/%oname/
%_includedir/%oname/*.h
%_includedir/%oname/*.hh
%_includedir/%oname/IOs/
%_includedir/%oname/modbus/
%_includedir/%oname/mysql/

%_libdir/libUniSet.so
#%_datadir/idl/%oname/
%_pkgconfigdir/libUniSet.pc

%files mysql-dbserver
%_bindir/%oname-mysql-*dbserver
%_libdir/*-mysql.so*

%if_enabled doc
%files doc
%_docdir/%name
%endif

%files extensions
%_bindir/%oname-iocontrol
%_bindir/%oname-iotest
%_bindir/%oname-iocalibr
%_bindir/%oname-logicproc
%_bindir/%oname-plogicproc
%_bindir/mtrconv
%_bindir/vtconv
%_bindir/rtustate
%_bindir/%oname-rtuexchange
%_bindir/%oname-smemory
%_bindir/%oname-smviewer
%_bindir/%oname-network

%_libdir/*Extensions.so.*
%_libdir/libUniSetIO*.so.*
%_libdir/libUniSetLP*.so.*
%_libdir/libUniSetMB*.so.*
%_libdir/libUniSetRT*.so.*
%_libdir/libUniSetShared*.so.*
%_libdir/libUniSetNetwork*.so.*

%files extensions-devel
%_includedir/%oname/extensions/
%_libdir/*Extensions.so
%_libdir/libUniSetIO*.so
%_libdir/libUniSetLP*.so
%_libdir/libUniSetMB*.so
%_libdir/libUniSetRT*.so
%_libdir/libUniSetShared*.so
%_libdir/libUniSetNetwork.so
%_pkgconfigdir/*Extensions.pc
%_pkgconfigdir/libUniSetIO*.pc
%_pkgconfigdir/libUniSetLog*.pc
%_pkgconfigdir/libUniSetMB*.pc
%_pkgconfigdir/libUniSetRT*.pc
%_pkgconfigdir/libUniSetShared*.pc
%_pkgconfigdir/libUniSetNetwork*.pc
#%_pkgconfigdir/libUniSet*.pc
%exclude %_pkgconfigdir/libUniSet.pc

%changelog
* Tue Sep 22 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter5
- minor fixes in SM

* Tue Sep 22 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter4
- minor fixes in IOBase

* Tue Sep 22 2009 Pavel Vainerman <pv@etersoft.ru> 0.97-eter3
- fixed bugs in IOControl

* Sat Sep 19 2009 Vitaly Lipatov <lav@altlinux.ru> 0.97-eter2
- fix build

* Wed Sep 16 2009 Vitaly Lipatov <lav@altlinux.ru> 0.97-eter1
- add getProp and getInt into generated _SK class for use default cnode
- UniXML: make xml2local, local2xml protected
- UniXML: add getContent for iterator (via xmlNodeGetContent)
- use logFile with string, without c_str
- UniXML: add getPropUtf8, findNodeUtf8, extFindNodeUtf8
- UniSetTypes: add getArgInt, getArgPInt
- forbid direct use atoi function in uniset and uniset related projects
- add string support for getIdByName
- use appropriate getArg(P)Int, get(P)IntProp instead direct atoi using
- Configuration: add getPIntField, getIntProp, getPIntProp, getArgPint
- Added byte size check in CycleStorage and TableBlockStorage
- add uniset-network (new component - UniNetwork)
- add new interface: getSensors()
- add new realization MBTCPMaster
- introduce getPintProp for get positive only values (returns def if the value zero or negative). Note: def may be negative if needed.

* Tue Sep 15 2009 Pavel Vainerman <pv@etersoft.ru> 0.96-eter63
- new build

* Mon Sep 07 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-eter54
- rebuild for new ModbusType parameters

* Mon Sep 07 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-eter53
- rebuild for new MBTCPMaster

* Sun Sep 06 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-eter52
- minor fixes in MBTCPMAster

* Fri Aug 21 2009 Pavel Vainerman <pv@etersoft.ru> 0.96-eter51
- minor fixes in RTUExchange

* Wed Aug 19 2009 Pavel Vainerman <pv@etersoft.ru> 0.96-eter50
- fixed bug in IOControl (blink mechanic)

* Wed Aug 19 2009 Pavel Vainerman <pv@etersoft.ru> 0.96-eter49
- add BLINK2, BLINK3 to IOControl

* Tue Aug 18 2009 Pavel Vainerman <pv@etersoft.ru> 0.96-eter48
- fixed bug in PassiveTimer

* Wed Aug 05 2009 Vitaly Lipatov <lav@altlinux.ru> 0.96-eter37
- fixed smp build

* Mon Aug 03 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-eter36
- new build

* Mon Aug 03 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-eter35
- new  RS  properties

* Sat Aug 01 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-eter30
- new build

* Tue Jul 14 2009 Vitaly Lipatov <lav@altlinux.ru> 0.96-eter29
- build from gear repo, rewrote spec
- rename extentions to extensions (see eterbug #4008)
- update buildreq

* Mon Apr 06 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-eter4
- new ComediInterface

* Mon Apr 06 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-eter3
- merge with mutabor/master

* Mon Apr 06 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-eter2
- fixed bugs in uniset-codegen

* Sat Jan 17 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-alt7
- new version

* Tue Dec 16 2008 Pavel Vainerman <pv@altlinux.ru> 0.96-alt1
- new version (+extensions)

* Tue Nov 27 2007 Pavel Vainerman <pv@altlinux.ru> 0.93-alt13
- new version

* Tue Nov 27 2007 Pavel Vainerman <pv@altlinux.ru> 0.93-alt11
- new version

* Sun Nov 04 2007 Pavel Vainerman <pv@altlinux.ru> 0.92-alt5
- new version

* Tue Oct 23 2007 Pavel Vainerman <pv@altlinux.ru> 0.92-alt4
- build for C30

* Wed Oct 17 2007 Pavel Vainerman <pv@altlinux.ru> 0.91-alt8
- new version

* Wed Oct 17 2007 Pavel Vainerman <pv@altlinux.ru> 0.91-alt7
- new version

* Fri Sep 14 2007 Pavel Vainerman <pv@altlinux.ru> 0.91-alt6
- new version

* Fri Aug 03 2007 Pavel Vainerman <pv@altlinux.ru> 0.91-alt5
- new build for C30

* Thu Aug 02 2007 Pavel Vainerman <pv@altlinux.ru> 0.91-alt4
- new version for C30

* Thu Aug 02 2007 Pavel Vainerman <pv@altlinux.ru> 0.91-alt3
- new version for C30

* Mon Jul 30 2007 Pavel Vainerman <pv@altlinux.ru> 0.91-alt1
- build for C30

* Fri Jul 13 2007 Pavel Vainerman <pv@altlinux.ru> 0.9-alt14.C30
- buid for C30

* Sun Jul 08 2007 Pavel Vainerman <pv@altlinux.ru> 0.9-alt13
- new build

* Sat Jul 07 2007 Pavel Vainerman <pv@altlinux.ru> 0.9-alt0.C30.1
- build for C30

* Tue Jun 19 2007 Pavel Vainerman <pv@altlinux.ru> 0.8-alt10.M30
- new version

* Mon Jun 18 2007 Pavel Vainerman <pv@altlinux.ru> 0.8-alt9.M30
- new version (for M30)

* Mon Jun 18 2007 Pavel Vainerman <pv@altlinux.ru> 0.8-alt8.M30
- new version (for M30)

* Mon Jun 18 2007 Pavel Vainerman <pv@altlinux.ru> 0.8-alt7
- new version (for M30)

* Mon Jun 18 2007 Pavel Vainerman <pv@altlinux.ru> 0.8-alt1
- new build

* Mon Jun 18 2007 Pavel Vainerman <pv@altlinux.ru> 0.7-alt5
- new build

* Sat Feb 17 2007 Pavel Vainerman <pv@altlinux.ru> 0.7-alt2
- new build

* Mon Feb 05 2007 Pavel Vainerman <pv@altlinux.ru> 0.7-alt1
- new build

* Mon Dec 25 2006 Pavel Vainerman <pv@altlinux.ru> 0.7-alt0.1
- new build

* Tue Jun 13 2006 Pavel Vainerman <pv@altlinux.ru> 0.6-alt2
- fix bug for gcc 4.1.0

* Tue Jun 13 2006 Pavel Vainerman <pv@altlinux.ru> 0.6-alt1
- new version

* Fri Sep 09 2005 Pavel Vainerman <pv@altlinux.ru> 0.5.RC5-alt2
- fixed bug ind AskDUmper::SInfo::operator=

* Mon Aug 29 2005 Pavel Vainerman <pv@altlinux.ru> 0.5.RC5-alt1
- detach mysql-dbserver
- add --disable-mysql for configure script

* Fri Jun 24 2005 Pavel Vainerman <pv@altlinux.ru> 0.5.RC1-alt1
- build new version
- fixed bugs

* Sat Mar 26 2005 Pavel Vainerman <pv@altlinux.ru> 0.4.9-alt4
- fixed bug in IOController: not registration child Objects
- add docs section for IOConfigure

* Sun Feb 27 2005 Pavel Vainerman <pv@altlinux.ru> 0.4.9-alt3
- change createNext in UniXML
- add copyNode (new function to UniXML)

* Tue Feb 22 2005 Pavel Vainerman <pv@altlinux.ru> 0.4.9-alt2
- bug fix for AskDumperXML1

* Mon Feb 21 2005 Pavel Vainerman <pv@altlinux.ru> 0.4.9-alt1
- add ClassGen utility

* Fri Feb 04 2005 Pavel Vainerman <pv@altlinux.ru> 0.4.8-alt1
- correct uniset-admin --logrotate function

* Sat Jan 29 2005 Pavel Vainerman <pv@altlinux.ru> 0.4.7-alt1
- compiled for gcc3.4

* Thu Jan 06 2005 Pavel Vainerman <pv@altlinux.ru> 0.4.6-alt1
- move idl files to /%_datadir/idl/%name
- remove old files

* Mon Jan 03 2005 Pavel Vainerman <pv@altlinux.ru> 0.4.4-alt1
- new version
- rename TimeService --> TimerService
- new start/stop scripts for local run

* Fri Dec 24 2004 Pavel Vainerman <pv@altlinux.ru> 0.4.1-alt1
- new version
- add analog and digital filters
- add sensibility for analog sensor

* Wed Dec 22 2004 Pavel Vainerman <pv@altlinux.ru> 0.0.4-alt1
- build new version

* Tue Nov 09 2004 Pavel Vainerman <pv@altlinux.ru> 0.0.2-alt1
- new version
- disable uniset.xxx-xxx.xxx.rpm

* Mon Nov 08 2004 Pavel Vainerman <pv@altlinux.ru> 0.0.1-alt1
- first build
