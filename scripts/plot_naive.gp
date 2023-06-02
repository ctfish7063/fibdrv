reset
set xlabel "fib(n)"
set ylabel "Time (ns)"
set title "Naive method"
set terminal png font " Times_New_Roman,12 "
set output "naive.png"
set autoscale
set key left
set key box

plot "naive.txt" using 1:2 with lines linewidth 2 title "kernel",\
"naive.txt" using 1:3 with lines linewidth 2 title "user",\
"naive.txt" using 1:4 with lines linewidth 2 title "user to kernel"
