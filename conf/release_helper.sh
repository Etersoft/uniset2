#!/bin/sh
# Вспомогательный скрипт для подготовки и сборки rpm-пакета с системой
. /etc/rpm/etersoft-build-functions

cd ..
SPECNAME=conf/uniset.spec
check_key

update_from_cvs

build_rpms_name $SPECNAME
export BUILDNAME=$BASENAME-$VERSION-$RELEASE
cvs update $SPECNAME ChangeLog && ./cvs2cl.pl && cvs commit -m "Auto updated by $0 for $BUILDNAME" ChangeLog || fatal "can't update Changelog"
add_changelog_helper "- new build" $SPECNAME

prepare_tarball

rpmbb $SPECNAME || fatal "Can't build"

DESTDIR=/var/ftp/pvt/Etersoft/Ourside/i586/RPMS.ourside/
build_rpms_name $SPECNAME
if [ -d ${DESTDIR} ] ; then
	rm -f $DESTDIR/$BASENAME*
	echo "Copying to $DESTDIR"
	cp $RPMDIR/RPMS/$DEFAULTARCH/$BASENAME-*$VERSION-$RELEASE.* $DESTDIR/ || fatal "Can't copying"
	chmod o+r $DESTDIR/$BASENAME-*
fi

# Увеличиваем релиз и запоминаем спек после успешной сборки
BASERELEASE=$(get_release $SPECNAME | sed -e "s/alt//")
set_release $SPECNAME alt$((BASERELEASE+1))
cvs commit -m "Auto updated by $0 for $BUILDNAME" $SPECNAME || fatal "Can't commit spec"

# Note: we in topscrdir
#TAG=${BUILDNAME//./_}
#echo "Set tag $TAG ..."
#cvs tag $TAG || fatal "Can't set build tag"
