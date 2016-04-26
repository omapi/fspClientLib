#!/bin/sh

export LD_LIBRARY_PATH=`pwd`/../
Server="-s 192.168.130.80"
if [ "$1" = "s" ];then
	shift
	./p2pdemo ${Server} -d -l 5 $*
elif [ "$1" = "c" ];then
	shift
	./p2pdemo ${Server} -d -l 5 -c $*
elif [ "$1" = "testconn" ];then
	# ./build.sh testconn 5             运行5个客户端，连接对应的服务
	# ./build.sh testconn 5 peer_s_3    运行5个客户端，连接同一个服务
	i=0
	while [[ $i -lt $2 ]]; do
		i=`expr $i + 1`
		if [ "$3" != "" ];then
			./p2pdemo ${Server} -d -N peer_c_$i -i peer_c_$i -c $3 &
		else
			./p2pdemo ${Server} -d -N peer_c_$i -i peer_c_$i -c peer_s_$i &
		fi
	done
	echo -e "$i endpoint start connect"
elif [ "$1" = "testreg" ];then
	# ./build.sh testreg 5               运行5个服务
	i=0
	while [[ $i -lt $2 ]]; do
		i=`expr $i + 1`
		./p2pdemo ${Server} -d -N peer_s_$i -i peer_s_$i &
	done
	echo -e "$i endpoint started"
elif [ "$1" = "lib" ];then
	cd ../; ./build.sh $2
elif [ "$1" != "" ];then
	if [ "$1" = "wroc" ];then
		export PATH=/opt/buildroot-gcc342/bin:$PATH
	elif [ "$1" = "om400" ];then
		export PATH=/opt/eabi/arm-2009q1/bin:/opt/eabi/arm-eabi-4.4.0/bin:$PATH
	fi
	make clean
	make TARGET=$1
fi

