Results:

![Raw times, b](./graph_rawtimes_b.png)

![Raw times, s](./graph_rawtimes_s.png)

![Ratios](./graph_ratios.png)

You can run the benchmark on your machine as follows:
```bash
# Run benchmark for pointer-and-length strings
./bench.sh b | tee RESULTS_b.txt

# Run benchmark for null-terminated strings
./bench.sh s | tee RESULTS_s.txt

# Generate 'graph_rawtimes_b.png'
gnuplot < graph_rawtimes_b.gnuplot

# Generate 'graph_rawtimes_s.png'
gnuplot < graph_rawtimes_s.gnuplot

# Generate 'graph_ratios.png'
gnuplot < graph_ratios.gnuplot
```
