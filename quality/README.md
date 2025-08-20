Results:

![Graph](./graph.png)

You can reproduce our results as follows:

```bash
# Compile evalqual
gcc -Wall -Wextra -O3 evalqual.c ../utils/*.c -lm -o evalqual

# Download and unpack the corpus (1.3 MB uncompressed)
wget -O- http://shdown.github.io/stuff/jjhash/check_quality_corpus.txt.gz | gzip -d > words.txt

# We want to evaluate quality only for the single JJ prime
echo 2752750471 > primes.txt

# Run evalqual
./evalqual words.txt primes.txt | tee data_raw.txt

# Process the raw data (generates files 'graph_data_fnv.txt' and 'graph_data_jj.txt')
./process.sh data_raw.txt

# Make the plot (generates file 'graph.png')
gnuplot < graph.gnuplot
```
