#!/bin/sh

USERID=0
BASEOMNIPORT=2809
[ -z "$UNISET_SCRIPT_VERBOSE" ] && UNISET_SCRIPT_VERBOSE=

# Получаем наш внутренний номер пользователя
function get_userid()
{
	USERID=$(expr $UID + 50000)
}

function uniset_msg()
{
    [ -z "$UNISET_SCRIPT_VERBOSE" ] && return
    echo $1 $2 $3
}

# usage: standart_control {1/0} - {standart port/debug port}
function standart_control()
{
	if [ -z $TMPDIR ]
	then
		TMPDIR=$HOME/tmp
		uniset_msg "Не определена переменная окружения TMPDIR. Используем $TMPDIR"
	else
		uniset_msg "Определена TMPDIR=$TMPDIR"
	fi

	if [ $1 = 1 ]; then
		TMPDIR=/var/tmp
		uniset_msg  "Используем стандартный порт Omni: $BASEOMNIPORT и временный каталог $TMPDIR"
	else
		get_userid
		if [ $USERID = 0 ]
		then
			uniset_msg "Не разрешено запускать пользователю $(whoami) с uid=$UID"
			exit 1
		fi
	fi
}

function set_omni_port
{
	while [ -n "$1" ]; do
		case "$1" in
			-p|--port)
				shift
					OMNIPORT=$1;
					uniset_msg "set OMNIPORT=$1"
				shift;
				break;
			;;

			*)
				shift
			;;
		esac
	done
}

function set_omni
{
	# Каталог для хранения записей omniORB
	OMNILOG=$TMPDIR/omniORB

	# Файл для хранения перечня запущенных в фоновом режиме процессов
	RANSERVICES=$OMNILOG/ran.list
	touch $RANSERVICES

	OMNINAME=omniNames

	OMNIPORT=$(expr $USERID + $BASEOMNIPORT)

	if [ $(grep -q "$OMNIPORT/" /etc/services | wc -l) \> 0 ]
	then
		if [ $USERID = 0 ]
		then
			uniset_msg "INFO: Запись о порте $OMNIPORT присутствует в /etc/services."
		else
			uniset_msg "Извините, порт $OMNIPORT уже присутствует в /etc/services."
			uniset_msg "Запуск omniNames невозможен."
			uniset_msg "Завершаемся"
			exit 0
		fi
	fi
	[ -e $(which $OMNINAME) ]  || { uniset_msg "Error: Команда $OMNINAME не найдена" ; exit 0; }

}


function runOmniNames()
{
	RETVAL=1
	omniTest=0
	if [ $std = 1 ]; then
		omniTest=$(ps ax | grep -q $OMNINAME | grep -v 'grep' | grep -v $0 | wc -l);
	else
		omniTest=$(ps aux | grep -q $OMNINAME | grep "$USER"  | grep -v 'grep' | grep -v $0 | wc -l);
	fi

	if [ $omniTest \> 0 ];
	then
		uniset_msg "$OMNINAME уже запущен. #Прерываем."
		return 0;
	fi

	if [ ! -d $OMNILOG ]
	then
		mkdir -p $OMNILOG
		uniset_msg "Запуск omniNames первый раз с портом $OMNIPORT"
		$OMNINAME -start $OMNIPORT -logdir $OMNILOG &>$OMNILOG/background.output &
		pid=$!
		uniset_msg "Создание структуры репозитория объектов"
	else
		uniset_msg "Обычный запуск omniNames. Если есть проблемы, сотрите $OMNILOG"
		$OMNINAME -logdir $OMNILOG &>$OMNILOG/background.output &
		pid=$!
	fi
	RET=$?
	if [ $RET = 0 ]; then
		if [ $WITH_PID = 1 ]; then
			echo $pid >"$RUNDIR/$OMNINAME.pid" # создаём pid-файл
		fi;
	else
		uniset_msg "Запуск omniNames не удался"
		return 1;
	fi
	#echo $! $OMNINAME >>$RANSERVICES

	if [ $(grep -q $OMNINAME $RANSERVICES | wc -l) \= 0 ]
	then
		echo 0 $OMNINAME >>$RANSERVICES
	fi

	# Проверка на запуск omniNames -а
	yes=$(echo $* | grep omniNames )
	if [ -n "$yes" ]; then
		uniset_msg "Запуск omniNames [ OK ]"
		$RETVAL=0
	fi

	return $RETVAL
}
