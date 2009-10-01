#!/bin/sh
# общие функции
. uniset-functions.sh

WITH_PID=0
std=0
standart_control $std
set_omni
set_omni_port $*

runOmniNames

case $1 in
	--foreground|-f)
		shift 1
		COMLINE=$*
		if [ -z "$COMLINE" ]
		then
			echo "Не указана команда для запуска"
			exit 0
		fi
		
		COMLINE="$COMLINE --uniset-port $OMNIPORT"
		echo Запускаем "$COMLINE"
		$COMLINE 
		echo Выходим
		exit 1
		;;
esac

if [ -z "$*" ]
then
	echo "Не указана команда для запуска"
	exit 0
fi

	checkPID=$(echo "$1" | grep pidfile=)
	if [ -n "$checkPID" ]; then
		PIDFILE="$RUNDIR/${1#--pidfile=}"
		shift
		NAMEPROG="$1"
	else
		NAMEPROG="$1"
		PIDFILE="$RUNDIR/$(basename $NAMEPROG).pid"
	fi

	echo -n Запускаем $NAMEPROG в фоновом режиме...
	echo ""
    ulimit -S -c 0 >/dev/null 2>&1
#	$* --uniset-port $OMNIPORT &
	echo ЗАПУСК: "$* --uniset-port $OMNIPORT"
	
	pid=$!
	echo $pid >$PIDFILE # создаём pid-файл

	PROGLINE=$(ps -x | grep -q $(basename $NAMEPROG) | grep -v $0 | grep -v grep)

	if [ -n "$PROGLINE" ]; then
		RETVAL=1
	    echo [ OK ]
		echo $( echo $PROGLINE | cut -d " " -f 1 ) $NAMEPROG >>$RANSERVICES
	else
		RETVAL=0
		echo [ FAILED ] 
	fi

exit $RETVAL
