#!/bin/sh

while true
do
    Adafruit_DHT 22 17 | sed -e '1,2d' -e 's/[\ \*=\%a-zA-Z]//g' -e 's/,/'
    if [! -z "$vals" ]
        then 
        echo 'date + "%Y-%m-%d|%H:%M:%S"' $vals
    fi
done
