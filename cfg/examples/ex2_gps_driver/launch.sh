#!/bin/bash

if [ -z $1 ]; then
    echo "usage: launch.sh launch_list.txt" && exit 1
fi

set -e -u

trap "kill 0" SIGINT SIGTERM EXIT

IFS=$'\n'; 
i=0
for line in $(cat $1)
do
    printf "$line\n"
    xterm -hold -geometry 80x10+0+$(($i*180)) -e  "$line" & 
    sleep 0.1
    i=$(($i + 1))
done

printf "\nPress any key to exit and kill running processes\n"
read -n 1 -s
