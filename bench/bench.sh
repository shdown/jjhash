#!/usr/bin/env bash

set -e

awk_single_expr() {
    local expr=$1; shift
    awk "$@" "BEGIN { $expr; exit }"
}

do_bench() {
    local id=$1; shift

    local -a flags=( -O3 -Wall -Wextra -march=native )
    local -a files=( bench.c ../utils/{common,gen_word}.c )
    ${CC:-gcc} -DBENCH_FNV=1 "${flags[@]}" "${files[@]}" "$@" -o bench_fnv || return $?
    ${CC:-gcc} -DBENCH_FNV=0 "${flags[@]}" "${files[@]}" "$@" -o bench_jj || return $?

    local t_fnv
    t_fnv=$($PREFIX ./bench_fnv 2>/dev/null) || return $?

    local t_jj
    t_jj=$($PREFIX ./bench_jj 2>/dev/null) || return $?

    local ratio
    ratio=$(awk_single_expr 'printf("%.5f\n", a / b)' -v"a=$t_fnv" -v"b=$t_jj") || return $?

    echo -e "$id\t$ratio\t\t$t_fnv\t$t_jj"
}

what=${1?}; shift

if [[ $what == s ]]; then
    BENCH_B=0
elif [[ $what == b ]]; then
    BENCH_B=1
else
    echo >&2 "Unknown first argument '$what' (must be either 's' or 'b')."
    exit 1
fi

bench_for_n() {
    local i=$1; shift
    local n=$1; shift
    do_bench "$i $n" -DBENCH_WL=$n -DBENCH_B=$BENCH_B -DBENCH_NW=200 -DBENCH_NT=$(( 15000000 / n )) "$@"
}

for (( i = 3; i <= 23; ++i )); do
    j=$(awk_single_expr 'print(int(1.6 ** i))' -v"i=$i")
    if (( j % 4 )); then
        (( j = j - (j % 4) + 4 ))
    fi
    bench_for_n "$i" "$j" "$@"
done
