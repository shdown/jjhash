set terminal png size 840,630
set output 'graph_ratios.png'
set grid
set title "Speedup ratio, jjhash vs FNV"
set xlabel "log_{1.6}({/:Italic string length})" enhanced
set ylabel "Speedup factor ({/:Italic FNV time} / {/:Italic jjhash time})"
plot \
 'RESULTS_b.txt' using 1:3 with linespoints linewidth 2 linecolor 7 title 'pointer-and-length', \
 'RESULTS_s.txt' using 1:3 with linespoints linewidth 2 linecolor 6 title 'null-terminated'
