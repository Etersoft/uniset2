#!/bin/sh

# общие функции
. uniset-functions.sh

WITH_PID=0
std=0
standart_control $std
set_omni
set_omni_port $*
FG=
DBG=

runOmniNames

print_usage()
{
    [ "$1" = 0 ] || exec >&2
    cat <<EOF
Usage: ${0##*/} [options] programm

Valid options are:
  -h, --help	display help screen
  -f, --foreground   start programm on foreground. Default 'background'.

  -g, --gdb     start programm with gdb
  -vmem, --vg-memcheck        start programm with valgrind tool=memcheck
  -vcall, --vg-callgrind      start programm with valgrind tool=callgrind
  -vcache, --vg-cachegrind    start programm with valgrind tool=cachegrind
  -vhel, --vg-helgrind        start programm with valgrind tool=helgrind

EOF
    [ -n "$1" ] && exit "$1" || exit
}

#parse command line options
case "$1" in
	-h|--help) print_usage 0;;
    -f|--foreground) FG=1;;
	-vmem|--vg-memcheck) DBG="mem";;
	-vcall|--vg-callgrind) DBG="call";;
	-vcache|--vg-cachegrind) DBG="cache";;
    -vhel|--vg-helgrind) DBG="hel";;
	-g|--gdb) DBG="gdb";;
esac
shift

if [ -n "$DBG" ]
then
	COMLINE="$* --uniset-port $OMNIPORT"
	start_line=
	[ "$DBG" == "mem" ] && start_line="valgrind --tool=memcheck --leak-check=full --trace-children=yes --log-file=valgrind.log $COMLINE"
	[ "$DBG" == "call" ] && start_line="valgrind --tool=callgrind --trace-children=yes --log-file=valgrind.log $COMLINE"
	[ "$DBG" == "cache" ] && start_line="valgrind --tool=cachegrind --trace-children=yes --log-file=valgrind.log $COMLINE"
	[ "$DBG" == "hel" ] && start_line="valgrind --tool=helgrind --trace-children=yes --log-file=valgrind.log $COMLINE"

	if [ "$DBG" == "gdb" ]; then
        PRG="$1"; shift
		start_line="gdb --args $PRG -- $@"
	fi

	echo Running "$start_line"
    $start_line
	exit 0
fi

if [ -n "$FG" ]
then
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
fi

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
