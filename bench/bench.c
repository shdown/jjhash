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
#include "../utils/gen_word.h"
#include "../utils/fnv.h"

#include "../jjhash.h"

#define JJ(token) jjhash ## token

#define HASH_FUNC_ATTRS __attribute__((unused, noinline))

static HASH_FUNC_ATTRS uint32_t hash_fnv_b(const char *s, size_t ns)
{
    return FNV_b(s, ns);
}

static HASH_FUNC_ATTRS uint32_t hash_fnv_s(const char *s)
{
    return FNV_s(s);
}

static HASH_FUNC_ATTRS uint32_t hash_jj_b(const char *s, size_t ns)
{
    return JJ(_b)(s, ns);
}

static HASH_FUNC_ATTRS uint32_t hash_jj_s(const char *s)
{
    return JJ(_s)(s);
}

//-----------------------------------------------

#ifndef BENCH_B
#error "Please define BENCH_B."
#endif

#ifndef BENCH_NW
#error "Please define BENCH_NW."
#endif

#ifndef BENCH_NT
#error "Please define BENCH_NT."
#endif

#ifndef BENCH_FNV
#error "Please define BENCH_FNV."
#endif

#ifndef BENCH_WL
#error "Please define BENCH_WL."
#endif

#if BENCH_FNV
#define XXX_HASH_B hash_fnv_b
#define XXX_HASH_S hash_fnv_s
#else
#define XXX_HASH_B hash_jj_b
#define XXX_HASH_S hash_jj_s
#endif

typedef struct {
#if BENCH_B

# if BENCH_WL <= 256
#  define MAX_WORD_LEN (BENCH_WL - 1)
    char buf[BENCH_WL - 1];
    uint8_t len;

# elif BENCH_WL <= 65536
#  define MAX_WORD_LEN (BENCH_WL - 2)
    char buf[BENCH_WL - 2];
    uint16_t len;
# else
#  error "BENCH_WL > 65536 is not supported."
# endif

#else
# define MAX_WORD_LEN (BENCH_WL - 1)
    char buf[BENCH_WL];
#endif
} Word;

typedef struct {
    Word *data;
    size_t size;
    size_t capacity;
} Dict;

static Dict dict_new(size_t capacity)
{
    return (Dict) {
        .data = malloc_or_die(capacity, sizeof(Word)),
        .size = 0,
        .capacity = capacity,
    };
}

static void dict_add_raw(Dict *D, const Word *w)
{
    assert(D->size != D->capacity);
    Word *dst = &D->data[D->size++];
    memcpy(dst, w, sizeof(Word));
}

static void dict_add(Dict *D, const char *s)
{
    static Word w;

    size_t ns = strlen(s);

    assert(ns <= MAX_WORD_LEN);

    strncpy(w.buf, s, sizeof(w.buf));

#if BENCH_B
    w.len = ns;
#endif

    dict_add_raw(D, &w);
}

static uint32_t run_bench_once(Dict *D)
{
    Word *w = D->data;
    Word *w_end = w + D->size;

    uint32_t res = 0;
    for (; w != w_end; ++w) {
#if BENCH_B
        res ^= XXX_HASH_B(w->buf, w->len);
#else
        res ^= XXX_HASH_S(w->buf);
#endif
    }

    return res;
}

#define BARRIER_NOTHING() \
    asm volatile ("" ::: "memory")

#define BARRIER(X_) \
    do { \
        uint64_t x__ = (X_); \
        asm volatile ("" : : "r"(x__) : "memory"); \
    } while (0)

static inline uint64_t get_utime(void)
{
    BARRIER_NOTHING();

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t res = ((uint64_t) ts.tv_sec) * 1000000000 + ts.tv_nsec;

    BARRIER(res);
    return res;
}

int main()
{
    gen_word_global_init();

    Dict D = dict_new(BENCH_NW);

    for (int i = 0; i < BENCH_NW; ++i) {
        static char buf[BENCH_WL];

        size_t len = gen_word_len_almost_full(MAX_WORD_LEN);
        gen_word(buf, len);
        buf[len] = '\0';

        dict_add(&D, buf);
    }

    assert(D.size == D.capacity);

    uint64_t t0 = get_utime();

    //--------------------

    uint32_t summed_hashes = 0;
    for (int i = 0; i < BENCH_NT; ++i) {
        summed_hashes += run_bench_once(&D);
    }

    //--------------------

    uint64_t t = get_utime() - t0;

    printf("%.5f\n", ((double) t) / 1e9);

    fprintf(stderr, "summed_hashes=%" PRIu32 "\n", summed_hashes);
}
