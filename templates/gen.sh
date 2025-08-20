#!/usr/bin/env bash

set -e
set -x

gen() {
    local bits=$1

    local suffix
    local ctype
    local mask
    local comments_rule
    if (( bits == 32 )); then
        suffix=
        ctype=uint32_t
        mask=' \& UINT32_C(0xffffffff)'
        comments_rule='s/^@C//'
    else
        suffix=64
        ctype=uint64_t
        mask=
        comments_rule='/^@C/d'
    fi

    sed "${comments_rule}; s/@#/${suffix}/g; s/@T/${ctype}/g; s/@M/${mask}/g"
}

gen 32 < ./jjhash.tmpl  > ../jjhash.h
gen 32 < ./jjhashx.tmpl > ../jjhashx.h

gen 64 < ./jjhash.tmpl  > ../jjhash_64/jjhash64.h
gen 64 < ./jjhashx.tmpl > ../jjhash_64/jjhashx64.h
