reset
set xlabel "fib(n)"
set ylabel "Time (ns)"
set title "Fast Doubling"
set terminal png font " Times_New_Roman,12 "
set output "fast.png"
set autoscale
set key left
set key box

plot "fast.txt" using 1:2 with lines linewidth 2 title "kernel",\
"fast.txt" using 1:3 with lines linewidth 2 title "user",\
"fast.txt" using 1:4 with lines linewidth 2 title "user to kernel"
