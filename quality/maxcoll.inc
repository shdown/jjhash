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

#ifndef MAXCOLL_NAME
#error "You must define MAXCOLL_NAME."
#endif

#ifndef MAXCOLL_HASH
#error "You must define MAXCOLL_HASH."
#endif

#ifndef MAXCOLL_COLL_TYPE
#error "You must define MAXCOLL_COLL_TYPE."
#endif

#ifndef MAXCOLL_RESULT_TYPE
#error "You must define MAXCOLL_RESULT_TYPE."
#endif

#ifndef MAXCOLL_EVAL
#error "You must define MAXCOLL_EVAL."
#endif

static __attribute__((noinline)) void MAXCOLL_NAME(
    Corpus *C,
    MAXCOLL_RESULT_TYPE *output,
    uint8_t cur_H,
    uint8_t cur_H_MIN)
{
    enum { N = 1 << H };
    size_t cur_N = ((size_t) 1) << cur_H;

#if MAXCOLL_WITH_DYNALLOC
    MAXCOLL_COLL_TYPE *coll = calloc_or_die(cur_N, sizeof(MAXCOLL_COLL_TYPE));
#else
    static MAXCOLL_COLL_TYPE coll[N];
    memset(coll, 0, sizeof(coll));
#endif

    char *p = C->data;

    uint32_t mask = cur_N - 1;
    for (;;) {
        uint8_t len = *p;
        if (unlikely(!len)) {
            break;
        }
        ++p;
        uint32_t hash = MAXCOLL_HASH(p, len) & mask;
        ++coll[hash];
        p += len;
    }

    output[cur_H] = MAXCOLL_EVAL(coll, cur_N);

    for (size_t h = cur_H - 1; h >= cur_H_MIN; --h) {
        size_t n = 1 << h;
        for (size_t i = 0; i < n; ++i) {
            coll[i] += coll[i + n];
        }
        output[h] = MAXCOLL_EVAL(coll, n);
    }

#if MAXCOLL_WITH_DYNALLOC
    free(coll);
#endif
}