#!/usr/bin/env bash

set -e

do_process() {
    local cond
    if [[ $1 == fnv ]]; then
        cond='$1 == 0'
    else
        cond='$1 != 0'
    fi
    awk "$cond"'{ printf("%s %.20f\n", $2, $3 / 1.5 ** (30 - $2)) }'
}

input_file=${1?}

do_process fnv < "$input_file" > graph_data_fnv.txt
do_process jj < "$input_file" > graph_data_jj.txt
