#!/bin/sh
# Вспомогательный скрипт для подготовки и сборки rpm-пакета с системой

ETERBUILDVERSION=163
. /usr/share/eterbuild/eterbuild
load_mod spec

REL=eter
MAILDOMAIN=server
BACKUPDIR=$FTPDIR/backup
RPMBINDIR=$RPMDIR/RPMS
# builder50 path
TOPDIR=/var/ftp/pvt/Etersoft/Ourside/

PROJECT=$1
test -n "$PROJECT" || PROJECT=uniset

SPECNAME=libuniset.spec

FTPDIR=$TOPDIR/i586/RPMS.$PROJECT
if [ -d "$FTPDIR" ] ; then
	GEN="genbasedir --create --progress --topdir=$TOPDIR i586 $PROJECT"
else
	# For NoteBook build
	FTPDIR=/var/ftp/pub/Ourside/i586/RPMS.$PROJECT
	GEN=/var/ftp/pub/Ourside/i586/genb.sh
fi

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
	mkdir -p $BACKUPDIR
	mv -f $FTPDIR/*$PROJECT* $BACKUPDIR/
	mv -f $RPMBINDIR/*$PROJECT* $FTPDIR/
	chmod 'a+rw' $FTPDIR/*$PROJECT*
	$GEN
}


# ------------------------------------------------------------------------

add_changelog_helper "- new build" $SPECNAME

rpmbb $SPECNAME || fatal "Can't build"

cp2ftp

#rpmbs $SPECNAME
#send_notify

# Увеличиваем релиз и запоминаем спек после успешной сборки
inc_release $SPECNAME
# и запоминаем спек после успешной сборки
#cvs commit -m "Auto updated by $0 for $BUILDNAME" $SPECNAME || fatal "Can't commit spec"

# Note: we in topscrdir
#TAG=${BUILDNAME//./_}
#echo "Set tag $TAG ..."
#cvs tag $TAG || fatal "Can't set build tag"
