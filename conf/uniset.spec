%def_disable doc

Name: uniset
Version: 0.96
Release: eter18
Summary: UniSet
License: GPL
Group: Development/C++
URL: http://sourceforge.net/uniset
Packager: Pavel Vainerman <pv@altlinux.ru>
Source: %name-%version.tar.bz2

# Automatically added by buildreq on Sun Dec 28 2008
BuildRequires: libMySQL-devel libcomedi-devel libcommoncpp2-devel libomniORB-devel libsigc++2.0-devel libxml2-devel

#BuildRequires: gcc-c++ libMySQL-devel libomniORB-devel libsigc++2.0-devel libxml2-devel linux-libc-headers python-modules
#BuildRequires: gcc-c++ libMySQL-devel libomniORB-devel libsigc++2.0-devel libxml2-devel python-modules kernel-headers-common pkgconfig zlib-devel

%description
UniSet

%package -n lib%name
Summary: Libraries for UniSet
Group: System/Libraries

%description -n lib%name
This package provides libraries to use UniSet.

%package -n lib%name-devel
Group: Development/C
Summary: Libraries needed to develop for UniSet
Requires: lib%name = %version-%release

%description -n lib%name-devel
Libraries needed to develop for UniSet.

%package -n %name-mysql-dbserver
Group: Development/Databases
Summary: MySQL-dbserver implementatioin for UniSet
Requires: lib%name = %version-%release

%description -n %name-mysql-dbserver
MySQL dbserever for libuniset

%package utils
Summary: UniSet utilities
Group: Development/Tools
Requires: lib%name = %version-%release

%description utils
UniSet utilities 

%package -n lib%name-doc
Group: Development/C
Summary: Documentations for developing with UniSet.
Requires: lib%name = %version-%release

%description -n lib%name-doc
Documentations for developing with UniSet

%package -n lib%name-extentions
Group: Development/Databases
Summary: libUniSet extentions
Requires: lib%name = %version-%release

%description -n lib%name-extentions
Libraries needed to develop for UniSetExtentions.

%package -n %name-extentions
Group: Development/Databases
Summary: libUniSet extentions
Requires: lib%name-extentions = %version-%release

%description -n %name-extentions
Extentions for libuniset

%package -n lib%name-extentions-devel
Group: Development/Databases
Summary: Libraries needed to develop for uniset extentions
Requires: lib%name-extentions = %version-%release

%description -n lib%name-extentions-devel
Libraries needed to develop for uniset extentions

%prep
%setup -q

%build
%if_enabled doc
%configure
%else
%configure --disable-docs
%endif

#%make_build
%make

%install
%makeinstall

%post -n lib%name

%postun -n lib%name

%files utils
%_bindir/%name-admin
%_bindir/%name-infoserver
%_bindir/%name-mb*
%_bindir/%name-nullController
%_bindir/%name-sviewer-text
%_bindir/%name-smonit
%_bindir/%name-start*
%_bindir/%name-stop*
%_bindir/%name-func*
%_bindir/%name-codegen
%_datadir/%name/xslt/*.xsl

%files -n lib%name
%_libdir/libUniSet.so*
%exclude %_libdir/*Extentions.so*
%exclude %_libdir/libUniSetIO*.so*
%exclude %_libdir/libUniSetLP*.so*
%exclude %_libdir/libUniSetMB*.so*
%exclude %_libdir/libUniSetRT*.so*
%exclude %_libdir/libUniSetShared*.so*

%files -n lib%name-devel
%_includedir/%name
%_datadir/idl/%name
%_libdir/pkgconfig/libUniSet.pc
%exclude %_includedir/%name/extentions
#%exclude %_libdir/pkgconfig/libUniSet*.pc
%exclude %_libdir/pkgconfig/*Extentions.pc
%exclude %_libdir/pkgconfig/libUniSetIO*.pc
%exclude %_libdir/pkgconfig/libUniSetLog*.pc
%exclude %_libdir/pkgconfig/libUniSetMB*.pc
%exclude %_libdir/pkgconfig/libUniSetRT*.pc
%exclude %_libdir/pkgconfig/libUniSetShared*.pc

%files -n %name-mysql-dbserver
%_bindir/%name-mysql-*dbserver
%_libdir/*-mysql.so*

%files -n lib%name-doc
%if_enabled doc
%_docdir/%name
%endif

%files -n lib%name-extentions
%_libdir/*Extentions.so*
%_libdir/libUniSetIO*.so*
%_libdir/libUniSetLP*.so*
%_libdir/libUniSetMB*.so*
%_libdir/libUniSetRT*.so*
%_libdir/libUniSetShared*.so*


%files -n lib%name-extentions-devel
%_includedir/%name/extentions

%_libdir/pkgconfig/*Extentions.pc
%_libdir/pkgconfig/libUniSetIO*.pc
%_libdir/pkgconfig/libUniSetLog*.pc
%_libdir/pkgconfig/libUniSetMB*.pc
%_libdir/pkgconfig/libUniSetRT*.pc
%_libdir/pkgconfig/libUniSetShared*.pc
#%_libdir/pkgconfig/libUniSet*.pc
%exclude %_libdir/pkgconfig/libUniSet.pc

%files -n %name-extentions
%_bindir/%name-iocontrol
%_bindir/%name-iotest
%_bindir/%name-iocalibr
%_bindir/%name-logicproc
%_bindir/%name-plogicproc
%_bindir/mtrconv
%_bindir/rtustate
%_bindir/%name-rtuexchange
%_bindir/%name-smemory
%_bindir/%name-smviewer

%changelog
* Thu Jun 04 2009 Pavel Vainerman <pv@etersoft.ru> 0.96-eter18
- new build

* Thu Jun 04 2009 Pavel Vainerman <pv@etersoft.ru> 0.96-eter17
- new build

* Thu Jun 04 2009 Pavel Vainerman <pv@etersoft.ru> 0.96-eter16
- new build

* Thu Jun 04 2009 Pavel Vainerman <pv@etersoft.ru> 0.96-eter15
- new build

* Thu Jun 04 2009 Pavel Vainerman <pv@etersoft.ru> 0.96-eter14
- new build

* Thu Jun 04 2009 Pavel Vainerman <pv@etersoft.ru> 0.96-eter13
- new build

* Tue Apr 21 2009 Pavel Vainerman <pv@etersoft.ru> 0.96-eter6
- new build

* Tue Apr 21 2009 Pavel Vainerman <pv@etersoft.ru> 0.96-eter5
- new build

* Mon Apr 06 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-eter4
- new ComediInterface

* Mon Apr 06 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-eter3
- merge with mutabor/master

* Mon Apr 06 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-eter2
- fixed bugs in uniset-codegen

* Mon Apr 06 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-eter1
- new build

* Mon Apr 06 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-setri1
- new build

* Mon Apr 06 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-alt11
- new build

* Sat Jan 17 2009 Pavel Vainerman <pv@altlinux.ru> 0.96-alt7
- new version

* Tue Dec 16 2008 Pavel Vainerman <pv@altlinux.ru> 0.96-alt1
- new version (+extentions)

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
