#!/bin/bash

while true;
do
	unbuffer ./client 192.168.112.128 | (./halter 8 400000 ; killall client)
	/etc/init.d/tomcat6 restart
done
