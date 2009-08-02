#!/bin/sh
# Вспомогательный скрипт для подготовки и сборки rpm-пакета с системой
. /etc/rpm/etersoft-build-functions

cd ..
SPECNAME=conf/libuniset.spec
REL=eter
DEFAULTARCH=i586
RPMDIR=~/RPM/RPMS/$DEFAULTARCH
RPMSOURCEDIR=~/RPM/SOURCES
FTPDIR=/var/ftp/pub/Ourside/i586/RPMS.uniset
PROJECT=uniset
GEN=/var/ftp/pub/Ourside/i586/genb.sh
BACKUPDIR=$FTPDIR/backup
RPMBUILD=/usr/bin/rpmbuild

fatal()
{
	echo "Error: $@"
	exit 1
}

# Run inside project dir (named as name) (arg: local for noncvs build)
prepare_tarball()
{
	build_rpms_name $SPECNAME

	NAMEVER=$BASENAME-$VERSION
	WDPROJECT=$(pwd)
	TARNAME=$NAMEVER.tar.bz2
	DESTDIR=$TMPDIR/$NAMEVER
	RET=0

	mkdir -p $DESTDIR
	rm -rf $DESTDIR/*
	cp -r $WDPROJECT/* $DESTDIR/
	cd 	$DESTDIR/
		make distclean
		[ -a ./autogen.sh ] && ./autogen.sh
		rm -rf autom4te.cache/
	
		echo "Make tarball $TARNAME ... from $DESTDIR"
		mkdir -p $RPMSOURCEDIR/
		$NICE tar cfj $RPMSOURCEDIR/$TARNAME ../$NAMEVER $ETERTARPARAM || RET=1
		rm -rf $DESTDIR
	cd -

	[ $RET ] && echo "build ok" || fatal "Can't create tarball"
}

add_changelog_helper()
{
    tty -s || { echo "skip changelog fixing" ; return 1 ; }

	#FIXME HACK!! Приходится делать локаль POSIX, чтобы дата в changelog
	# вставилась на англ. языке. ИНАЧЕ её не пропускает rpmbb
	L="$LC_ALL"
	export LC_ALL=POSIX
	add_changelog -e "$@"
	R=$?
	export LC_ALL="$L"

	if [ "$R" = "0" ]; then
		shift
		for SPEC in "$@" ; do
			N=`grep -n '^%changelog' $SPEC | head -n 1 | sed s!:.*!!g`
			# +1 -- comment with date and packager name
			# +2 -- place for edit comments
			vim +$(($N + 2)) $SPEC
		done
	fi
	return $R
}

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
	mv -f $FTPDIR/$PROJECT* $BACKUPDIR/
	mv -f $RPMDIR/$PROJECT* $FTPDIR/
	chmod 'a+rw' $FTPDIR/$PROJECT*
	$GEN
}

# ------------------------------------------------------------------------
build_rpms_name $SPECNAME
export BUILDNAME=$BASENAME-$VERSION-$RELEASE

add_changelog_helper "- new build" $SPECNAME

#prepare_tarball || fatal "Can't prepare tarball"

rpmbb $SPECNAME || fatal "Can't build"

cp2ftp

rpmbs $SPECNAME
#send_notify

# Увеличиваем релиз и запоминаем спек после успешной сборки
BASERELEASE=$(get_release $SPECNAME | sed -e "s/$REL//")
set_release $SPECNAME ${REL}$((BASERELEASE+1))
#cvs commit -m "Auto updated by $0 for $BUILDNAME" $SPECNAME || fatal "Can't commit spec"

# Note: we in topscrdir
#TAG=${BUILDNAME//./_}
#echo "Set tag $TAG ..."
#cvs tag $TAG || fatal "Can't set build tag"
