#!/bin/sh
case "$1" in
     start)
        echo "Starting sensor data daemon"
        $(/home/abrahamx91/Professional/Git/CIS508/Lab/Lab4-3/gendata.sh >> /var/www/gnuplot_data) &
        ;;
    stop)
        echo "Shutting down data daemon"
        killall gendata.sh 
        ;;
    *)
        echo "Usage: $0 {start|stop}"
        exit 1
esac
exit 0
