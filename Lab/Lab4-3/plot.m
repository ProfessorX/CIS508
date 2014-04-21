set xdata time
set timefmt "%Y-%m-%d|%H:%M:%S"
set format x "%H:%M:%S"

set term pngcairo
set output "tmp.png"

plot "gnuplot_data" using 1:2 with linespoints title "Temperature"

set output "hum.png"
plot "gnuplot_data" using 1:3 with linespoints title "Humidity"
