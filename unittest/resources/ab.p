#output as png image
set terminal png

#save file to "domain.png"
set output "results/PATH/ab.png"

#graph title
set title "ab -n 1000 -c 50"

#nicer aspect ratio for image size
set size 1,0.7

# y-axis grid
set grid y

#x-axis label
set xlabel "request"

#y-axis label
set ylabel "response time (ms)"

#plot data from "domain.dat" using column 9 with smooth sbezier lines
#and title of "something" for the given data
plot "results/PATH/ab_plot" using 9 smooth sbezier with lines title "server"
