#!/usr/bin/env bash

set -e
set -x

for test_64 in 0 1; do
    ${CC:-gcc} -Wall -Wextra -O3 -g3 -DTEST_64="$test_64" ./validate.c ../utils/*.c -o validate
    ./validate
done
