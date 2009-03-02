#!/bin/sh
# Создание необходимых ссылок

ln -s -f admin.sh start
ln -s -f admin.sh exist
ln -s -f admin.sh finish
ln -s -f admin.sh foldUp
ln -s -f admin.sh info
ln -s -f admin.sh alarm
ln -s -f admin.sh create
#ln -s -f admin.sh setState
#ln -s -f admin.sh dbcreate
#ln -s -f admin.sh statistic
#ln -s -f admin.sh database
ln -s -f admin.sh logrotate
ln -s -f admin.sh omap
ln -s -f admin.sh msgmap
ln -s -f admin.sh anotify
ln -s -f admin.sh dnotify
ln -s -f admin.sh saveValue
ln -s -f admin.sh saveState
ln -s -f admin.sh setValue
ln -s -f admin.sh setState
ln -s -f admin.sh getValue
ln -s -f admin.sh getState
ln -s -f admin.sh getRawValue
ln -s -f admin.sh getCalibrate
ln -s -f admin.sh help
ln -s -f admin.sh oinfo

ln -s -f /usr/bin/uniset-stop.sh stop.sh
ln -s -f ../../conf/test.xml test.xml
