#!/bin/bash
set -e

if [[ -z $1 ]]; then
    driver=1
elif [[ $1 == "mmdriver" ]]; then
    driver=2
else
    echo "Error: invalid command line option: $1" && exit 1
fi

rm -f mm*.moos 

for i in 1 2
do 
cpp -D_MODEM_ID_=$i -D_DRIVER_=$driver mm.moos.in | sed -e 's/#.*//' -e 's/[ ^I]*$//' -e '/^$/ d' > mm$i.moos 
done

