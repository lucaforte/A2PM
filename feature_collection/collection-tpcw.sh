#!/bin/bash

while true;
do
	unbuffer ./lient 192.168.112.129 | (./halter 8 600000 ; killall client)
	/etc/init.d/tomcat6 restart
done
