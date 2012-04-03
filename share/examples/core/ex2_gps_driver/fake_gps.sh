#!/bin/bash

if [ -z $1 ]; then
    echo "usage: fake_gps.sh nmea.txt" && exit 1
fi

if [ -e "`which socat`" ]; then
    echo "Faking GPS on /tmp/ttyFAKE"
    while [ 1 ]; do 
        IFS=$'\n'; 
        for line in $(cat $1)
        do

            printf "$line\r\n" > /dev/stdout; 
            printf "$line\r\n" > /dev/stderr; 
            sleep 1;
        done
    done | socat - pty,raw,echo=0,link=/tmp/ttyFAKE
else
   echo "Missing socat; try sudo apt-get install socat"
fi
