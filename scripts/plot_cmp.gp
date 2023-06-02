reset
set xlabel "fib(n)"
set ylabel "Time (ns)"
set title "Performance"
set terminal png font " Times_New_Roman,12 "
set output "perf.png"
set autoscale
set key left
set key box

plot "fast.txt" using 1:2 with lines linewidth 2 title "fast_k",\
"fast.txt" using 1:3 with lines linewidth 2 title "fast_u",\
"naive.txt" using 1:2 with lines linewidth 2 title "naive_k",\
"naive.txt" using 1:3 with lines linewidth 2 title "naive_u"
