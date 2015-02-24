#!/bin/bash

while true;
do
	unbuffer ./client 217.111.119.142 | (./halter 8 2800000 ; killall client)
	sudo service tomcat6 restart
done
