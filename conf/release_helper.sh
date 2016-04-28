#!/bin/sh
# Вспомогательный скрипт для подготовки и сборки rpm-пакета с системой

ETERBUILDVERSION=163
. /usr/share/eterbuild/eterbuild
load_mod spec

REL=eter
MAILDOMAIN=server

# builder50 path
TOPDIR=/var/ftp/pvt/Etersoft/Ourside/

PKGNAME=uniset2
SPECNAME=libuniset2.spec

PLATFORM=i586
[[ `uname -m` == "x86_64" ]] && PLATFORM=x86_64

PROJECT=$1
test -n "$PROJECT" || PROJECT=$PKGNAME

if [ -d "$TOPDIR" ] ; then
	GEN="genbasedir --create --progress --topdir=$TOPDIR $PLATFORM $PROJECT"
else
	# For NoteBook build
	TOPDIR=/var/ftp/pub/Ourside
	GEN=/var/ftp/pub/Ourside/$PLATFORM/genb.sh
fi
FTPDIR=$TOPDIR/$PLATFORM/RPMS.$PROJECT
BACKUPDIR=$FTPDIR/backup

fatal()
{
	echo "Error: $@"
	exit 1
}

function send_notify()
{
	export EMAIL="$USER@$MAILDOMAIN"
	CURDATE=`date`
	MAILTO="devel@$MAILDOMAIN"
# FIXME: проверка отправки
mutt $MAILTO -s "[$PROJECT] New build: $BUILDNAME" <<EOF
Готова новая сборка: $BUILDNAME
-- 
your $0
$CURDATE
EOF
echo "inform mail sent to $MAILTO"
}

function cp2ftp()
{
	RPMBINDIR=$RPMDIR/RPMS
	test -d $RPMBINDIR/$PLATFORM && RPMBINDIR=$RPMBINDIR/$PLATFORM
	mkdir -p $BACKUPDIR
	mv -f $FTPDIR/*$PKGNAME* $BACKUPDIR/
	mv -f $RPMBINDIR/*$PKGNAME* $FTPDIR/
	chmod 'a+rw' $FTPDIR/*$PKGNAME*
	$GEN
}


# ------------------------------------------------------------------------

add_changelog_helper "- new build" $SPECNAME

rpmbb $SPECNAME || fatal "Can't build"

cp2ftp

rpmbs $SPECNAME
#send_notify

# Увеличиваем релиз и запоминаем спек после успешной сборки
# inc_release $SPECNAME
# и запоминаем спек после успешной сборки
#cvs commit -m "Auto updated by $0 for $BUILDNAME" $SPECNAME || fatal "Can't commit spec"

# Note: we in topscrdir
#TAG=${BUILDNAME//./_}
#echo "Set tag $TAG ..."
#cvs tag $TAG || fatal "Can't set build tag"
