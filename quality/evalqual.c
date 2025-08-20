/*
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <https://unlicense.org>
 */

#include "../utils/common.h"
#include "../utils/prng.h"
#include "../utils/fnv.h"
#include <math.h>

#define MAXCOLL_WITH_DYNALLOC 1
#define MAXCOLL_RESULT_TYPE uint64_t
#define MAXCOLL_COLL_TYPE uint32_t

static uint64_t JJ_PRIME;

#include "hash_jj.inc"

enum {
    H_MIN = 1,
    H = 30,
};

static PRNG global_prng;

typedef struct {
    char *data;
    size_t size;
    size_t capacity;
    size_t nwords;
} Corpus;

static void add_to_corpus_b(Corpus *C, const char *s, size_t ns)
{
    assert(ns < 256);

    while (C->capacity - C->size < ns + 1) {
        C->data = x2realloc_or_die(C->data, &C->capacity, sizeof(char));
    }

    char *p = C->data + C->size;
    p[0] = ns;

    if (ns) {
        memcpy(p + 1, s, ns);
    }

    C->size += ns + 1;

    if (ns) {
        ++C->nwords;
    }
}

static void add_to_corpus_s(Corpus *C, const char *s)
{
    add_to_corpus_b(C, s, strlen(s));
}

static inline MAXCOLL_RESULT_TYPE my_chi_sq(const MAXCOLL_COLL_TYPE *data, size_t ndata)
{
    uint64_t r = 0;
    for (size_t i = 0; i < ndata; ++i) {
        uint64_t b = data[i];
        r += b * (b + 1);
    }
    return r;
}

#define MAXCOLL_EVAL my_chi_sq

#define MAXCOLL_NAME maxcoll_fnv
#define MAXCOLL_HASH FNV_b
#include "maxcoll.inc"
#undef MAXCOLL_NAME
#undef MAXCOLL_HASH

#define MAXCOLL_NAME maxcoll_jj
#define MAXCOLL_HASH hash_jj
#include "maxcoll.inc"
#undef MAXCOLL_NAME
#undef MAXCOLL_HASH

typedef void (*maxcoll_func)(Corpus *C, MAXCOLL_RESULT_TYPE *output, uint8_t cur_H, uint8_t cur_H_MIN);

static double calc_chi2_rating(uint64_t xchi2, uint8_t rank, double nwords)
{
    xchi2 /= 2;
    double n = nwords;
    double m = 1ull << rank;
    double denom = n / (2 * m) * (n + 2 * m - 1);
    return xchi2 / denom;
}

static Corpus *corpi[H + 1];
static Corpus corpi_storage[H + 1];

static size_t *make_corpus_index(Corpus *C)
{
    size_t *res = malloc_or_die(C->nwords, sizeof(size_t));
    const char *p_begin = C->data;
    const char *p = p_begin;
    size_t i = 0;
    for (;;) {
        uint8_t len = (uint8_t) *p;
        if (unlikely(!len)) {
            break;
        }
        res[i++] = p - p_begin;
        p += len + 1;
    }
    assert(i == C->nwords);

    return res;
}

static void shuffle_J(size_t *J, size_t n)
{
#define SWAP(Type_, X_, Y_) \
    do { \
        Type_ tmp__ = (X_); \
        (X_) = (Y_); \
        (Y_) = tmp__; \
    } while (0)

    if (!n) {
        return;
    }
    size_t n_minus_one = n - 1;
    for (size_t i = 0; i < n_minus_one; ++i) {
        size_t j = i + prng_next_limit(&global_prng, n - i);
        SWAP(size_t, J[i], J[j]);
    }

#undef SWAP
}

static void downsample_corpus(Corpus *dst, Corpus *src, size_t nwords)
{
    size_t *src_index = make_corpus_index(src);

    size_t N_src = src->nwords;
    size_t *J = malloc_or_die(N_src, sizeof(size_t));
    for (size_t i = 0; i < N_src; ++i) {
        J[i] = i;
    }

    shuffle_J(J, N_src);

    *dst = (Corpus) {0};
    for (size_t i = 0; i < nwords; ++i) {
        size_t pos = src_index[J[i]];
        const char *s = src->data + pos;
        add_to_corpus_b(dst, s + 1, (uint8_t) s[0]);
    }
    add_to_corpus_s(dst, "");

    free(src_index);
    free(J);
}

static Corpus *get_corpus_for_h(Corpus *C, size_t h)
{
    Corpus *existing = corpi[h];
    if (existing) {
        return existing;
    }

    size_t nwords = ((size_t) 1) << h;

    if (nwords >= C->nwords) {
        corpi[h] = C;
        return C;
    }

    Corpus *res = &corpi_storage[h];
    downsample_corpus(res, C, nwords);
    corpi[h] = res;
    return res;
}

static size_t get_corpus_nwords_for_h(size_t h, size_t main_corpus_nwords)
{
    Corpus *existing = corpi[h];
    assert(existing);
    size_t res = existing->nwords;
    assert(res <= main_corpus_nwords);
    return res;
}

static inline __attribute__((always_inline))
void report_stats(Corpus *C, maxcoll_func f, uint64_t prime)
{
    MAXCOLL_RESULT_TYPE results[H + 1];

    for (size_t i = H_MIN; i <= H; ++i) {
        Corpus *C_cur = get_corpus_for_h(C, i);
        f(C_cur, results, i, i);
    }

    for (size_t i = H_MIN; i <= H; ++i) {
        size_t nwords = get_corpus_nwords_for_h(i, C->nwords);
        double chi2 = calc_chi2_rating(results[i], i, nwords);
        double res = log2(chi2);
        printf("%" PRIu64 " %zu %.20lf\n", prime, i, res);
        fflush(stdout);
    }
}

static void rstrip_nl(char *s)
{
    size_t ns = strlen(s);
    if (ns && s[ns - 1] == '\r') {
        --ns;
    }
    if (ns && s[ns - 1] == '\n') {
        --ns;
    }
    s[ns] = '\0';
}

static void read_corpus_or_die(Corpus *C, FILE *f)
{
    char *buf = NULL;
    size_t nbuf = 128;
    while (getline(&buf, &nbuf, f) >= 0) {
        rstrip_nl(buf);
        assert(buf[0]);
        add_to_corpus_s(C, buf);
    }

    free(buf);

    add_to_corpus_s(C, "");
}

static FILE *fopen_or_die(const char *path, const char *mode)
{
    FILE *f = fopen(path, mode);
    if (!f) {
        perror(path);
        abort();
    }
    return f;
}

typedef struct {
    uint64_t *data;
    size_t size;
    size_t capacity;
} Primes;

static inline void primes_add(Primes *P, uint64_t prime)
{
    if (P->size == P->capacity) {
        P->data = x2realloc_or_die(P->data, &P->capacity, sizeof(uint64_t));
    }
    P->data[P->size++] = prime;
}

static void actual_main(const char *words_file, const char *primes_file)
{
    Corpus C = {0};
    {
        FILE *f = fopen_or_die(words_file, "r");
        read_corpus_or_die(&C, f);
        fclose(f);
    }

    report_stats(&C, maxcoll_fnv, 0);

    Primes P = {0};
    {
        FILE *f = fopen_or_die(primes_file, "r");
        uint64_t prime;
        while (fscanf(f, "%" SCNu64 "\n", &prime) == 1) {
            primes_add(&P, prime);
        }
        fclose(f);
    }

    for (size_t i = 0; i < P.size; ++i) {
        JJ_PRIME = P.data[i];
        report_stats(&C, maxcoll_jj, JJ_PRIME);
    }
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "USAGE: evalqual WORDS_FILE PRIMES_FILE\n");
        return 2;
    }

    prng_init(&global_prng, 3059960585939474353ull);

    actual_main(argv[1], argv[2]);

    return 0;
}
