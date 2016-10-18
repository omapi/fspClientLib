#!/bin/sh

do_export()
{
	if [ "`uname -p`" = "x86_64" ];then
		export LD_LIBRARY_PATH=`pwd`/../lib/x64
	else
		export LD_LIBRARY_PATH=`pwd`/../lib/x86
	fi
}

do_export
echo -e "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"
Server="-s 192.168.130.80"
if [ "$1" = "s" ];then
	shift
	./p2pdemo ${Server} -d -l 5 $*
elif [ "$1" = "c" ];then
	shift
	./p2pdemo ${Server} -d -l 5 -c $*
elif [ "$1" = "r" ];then
	shift
	./p2pdemo ${Server} $*
elif [ "$1" = "lib" ];then
	cd ../; ./build.sh $2
elif [ "$1" != "" ];then
	if [ "$1" = "all" ];then
		$0 wroc
		$0 mx60
		$0 mx8
		$0 om400
		exit
	fi
	echo -e "**********build $1*********"
	if [ "$1" = "wroc" ];then
		export PATH=/opt/buildroot-gcc342/bin:$PATH
	elif [ "$1" = "om400" ];then
		export PATH=/opt/eabi/arm-2009q1/bin:/opt/eabi/arm-eabi-4.4.0/bin:$PATH
	fi
	make clean
	make TARGET=$1 STATIC=$2
	[ -f p2pdemo ] && cp p2pdemo p2pdemo_$1
fi

