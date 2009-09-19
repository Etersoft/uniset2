#!/bin/sh
# Вспомогательный скрипт для подготовки и сборки rpm-пакета с системой
. /etc/rpm/etersoft-build-functions

cd ..
SPECNAME=conf/libuniset.spec
RPMDIR=~/RPM/RPMS/$DEFAULTARCH
RPMSOURCEDIR=~/RPM/SOURCES
FTPDIR=/var/ftp/pub/Ourside/i586/RPMS.uniset
PROJECT=uniset
GEN=/var/ftp/pub/Ourside/i586/genb.sh
BACKUPDIR=$FTPDIR/backup

function send_notify()
{
	export EMAIL="$USER@server"
	CURDATE=`date`
	MAILTO="devel@server"
# FIXME: проверка отправки
mutt $MAILTO -s "[uniset] New build: $BUILDNAME" <<EOF
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
	mv -f $RPMDIR/*$PROJECT* $FTPDIR/
	chmod 'a+rw' $FTPDIR/*$PROJECT*
	$GEN
}

# ------------------------------------------------------------------------
build_rpms_name $SPECNAME
export BUILDNAME=$BASENAME-$VERSION-$RELEASE

add_changelog_helper "- new build" $SPECNAME

#prepare_tarball || fatal "Can't prepare tarball"

rpmbb $SPECNAME || fatal "Can't build"

cp2ftp

#rpmbs $SPECNAME
#send_notify

# Увеличиваем релиз
inc_release $SPECNAME
# и запоминаем спек после успешной сборки
#cvs commit -m "Auto updated by $0 for $BUILDNAME" $SPECNAME || fatal "Can't commit spec"

# Note: we in topscrdir
#TAG=${BUILDNAME//./_}
#echo "Set tag $TAG ..."
#cvs tag $TAG || fatal "Can't set build tag"
