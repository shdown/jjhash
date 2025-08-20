set terminal png size 800,630
set output 'graph.png'
set grid
set title "χ^2 statistics against perfect distribution, less is better"
set xlabel "{/:Italic h}, number of least significant bits to test\n\nD(h) = 1.5^{30-h}   (see Y axis label on the left)" enhanced
set ylabel "log_2(χ^2) / D(h)" enhanced
plot \
 'graph_data_fnv.txt' using 1:2 with linespoints linewidth 2 title 'FNV', \
 'graph_data_jj.txt' using 1:2 with linespoints linewidth 2 title 'jjhash'
