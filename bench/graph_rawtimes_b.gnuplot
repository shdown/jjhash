set terminal png size 800,630
set output 'graph_rawtimes_b.png'
set grid
set title "Execution time on pointer-and-length strings, less is better"
set xlabel "log_{1.6}({/:Italic string length})" enhanced
set ylabel "Time, s"
plot \
 'RESULTS_b.txt' using 1:4 with linespoints linewidth 2 title 'FNV', \
 'RESULTS_b.txt' using 1:5 with linespoints linewidth 2 title 'jjhash'
