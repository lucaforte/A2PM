#!/bin/bash


repeat=$(cat /home/user/collection/repeat)

if [ "$repeat" -eq "1" ]; then
	echo "Lo script si disabiliterà ora..."
	exit 0;
else
	echo "Avvio il ciclo di trasmissione"
fi

let repeat=$repeat+1
echo $repeat > /home/user/collection/repeat


sleep 20;

while true;
do
	unbuffer ./feature_client 192.168.199.1 | (./halter 8 600000 ; killall feature_client)
	/etc/init.d/tomcat6 restart
#	sudo reboot
done
