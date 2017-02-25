#!/bin/sh

GVB_SRC_IP=127.0.0.1
GVB_SRC_PORT=57777

GVB_RELAY_IP=192.168.1.40
GVB_RELAY_PORT=57777

#
# Трансляция по сети. Нужно поставить в цикл
#
while [ true ] 
do 

    nc -zv $GVB_SRC_IP $GVB_SRC_PORT

	if [ $? -eq 0 ]
	then
		gst-launch-1.0 -v \
		tcpclientsrc \
			  typefind=true \
			  host=$GVB_SRC_IP \
			  port=$GVB_SRC_PORT \
			  do-timestamp=true \
		  ! video/x-h264,width=1024,height=768 \
		  ! h264parse \
			  config-interval=-1 \
		  ! mpegtsmux \
		  ! tcpserversink \
			  host=$GVB_RELAY_IP \
			  port=$GVB_RELAY_PORT
#		break
	else
		sleep 3
	fi

done

