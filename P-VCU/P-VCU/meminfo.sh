#/bin/bash

while getopts "f:" opt
do
	case ${opt} in
		f)
			file=${OPTARG}
			;;
		\?)
			exit -1
			;;
	esac
done

if [ $file ]; then
	rm -f $file
fi

while true
do
	app_pid=`pidof dp_v6_door`
	if [ ! $app_pid ]; then
		echo "dp_v6_door not found, memory watching exits"
		break;
	fi

	if [ $file ]; then
		date "+%Y-%m-%d %H:%M:%S" >> $file
        	cat /proc/$app_pid/status | grep -i rss >> $file
		echo >> $file
	else
		date "+%Y-%m-%d %H:%M:%S"
        	cat /proc/$app_pid/status | grep -i rss
		echo
	fi

	sleep 1
done
