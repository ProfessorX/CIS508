#!/bin/bash
gnuplot "plot.m"
echo "Content-type: text/html"
echo ""
 
echo "<html>
        <body>
 
                <h2> Plots</h2>
                <img src='tmp.png' />
                <img src='hum.png' />
 
                <form action='index.cgi'>
                        <button type='submit' name='q'  value='0'> off </button>
                        <button type='submit' name='q'  value='1'> on </button>
                </form>
"
######## complete this part ########
# The gpio port shall be changed to fit. 
status=`cat /sys/class/gpio/gpioXX/value`
echo "<h2> LED status: $status </h2>"
 
cmd=`echo $QUERY_STRING | sed 's/[q=]//g' `

case in ......
 
 
####################################
 
echo "
        </body>
</html> "