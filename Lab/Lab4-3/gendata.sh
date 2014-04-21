#!/bin/sh

while true
do
    Adafruit_DHT 22 17 | sed -e '1\t2d' -e 's/[\ \*=\%a-zA-Z]//g'
    if [! -z "$vals" ]
        then 
        echo 'date + "%Y-%m-%d|%H:%M:%S"' $vals
    fi
done
