#!/bin/sh
USERID=0
BASEOMNIPORT=2809

# Получаем наш внутренний номер пользователя
function get_userid()
{
	USERID=$(expr $UID + 50000)
}

# usage: standart_control {1/0} - {standart port/debug port}
function standart_control()
{
	if [ -z $TMPDIR ]
	then
		TMPDIR=$HOME/tmp
		echo Не определена переменная окружения TMPDIR. Используем $TMPDIR
	else
		echo Определена TMPDIR=$TMPDIR
	fi

	if [ $1 = 1 ]; then
		TMPDIR=/var/tmp
		echo Используем стандартный порт Omni: $BASEOMNIPORT и временный каталог $TMPDIR
	else
		get_userid
		if [ $USERID = 0 ]
		then
			echo Не разрешено запускать пользователю $(whoami) с uid=$UID
			exit 0
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
					echo  "set OMNIPORT=$1" 
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

	if [ $(grep "$OMNIPORT/" /etc/services | wc -l) \> 0 ]
	then
		if [ $USERID = 0 ]
		then
			echo INFO: Запись о порте $OMNIPORT присутствует в /etc/services.
		else
			echo Извините, порт $OMNIPORT уже присутствует в /etc/services.
			echo Запуск omniNames невозможен.
			echo Завершаемся
			exit 0
		fi
	fi
	[ -e $(which $OMNINAME) ]  || { echo Error: Команда $OMNINAME не найдена ; exit 0; }

}


function runOmniNames()
{
	RETVAL=1
	omniTest=0
	if [ $std = 1 ]; then
		omniTest=$(ps -ax | grep $OMNINAME | grep -v grep | grep -v $0 | wc -l);
	else
		omniTest=$(ps -aux | grep $OMNINAME | grep $USER  | grep -v grep | grep -v $0 | wc -l);
	fi

	if [ $omniTest \> 0 ]; 
	then	 
		echo $OMNINAME уже запущен. #Прерываем.
		return 0;
	fi

	if [ ! -d $OMNILOG ]
	then
		mkdir -p $OMNILOG
		echo Запуск omniNames первый раз с портом $OMNIPORT
		$OMNINAME -start $OMNIPORT -logdir $OMNILOG &>$OMNILOG/background.output &
		pid=$!
		echo Создание структуры репозитория объектов
	else
		echo Обычный запуск omniNames. Если есть проблемы, сотрите $OMNILOG
		$OMNINAME -logdir $OMNILOG &>$OMNILOG/background.output &
		pid=$!
	fi
	RET=$?
	if [ $RET = 0 ]; then
		if [ $WITH_PID = 1 ]; then
			echo $pid >"$RUNDIR/$OMNINAME.pid" # создаём pid-файл
		fi;
	else
		echo Запуск omniNames не удался
		return 1;
	fi
	#echo $! $OMNINAME >>$RANSERVICES
	
	if [ $(grep $OMNINAME $RANSERVICES | wc -l) \= 0 ]
	then
		echo 0 $OMNINAME >>$RANSERVICES
	fi

	# Проверка на запуск omniNames -а
	yes=$(echo $* | grep omniNames )
	if [ -n "$yes" ]; then
		echo Запуск omniNames [ OK ]
		$RETVAL=0
	fi

	return $RETVAL
}