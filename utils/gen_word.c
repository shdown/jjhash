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

#include "gen_word.h"
#include "prng.h"

static PRNG prng;

void gen_word_global_init(void)
{
    prng_init(&prng, 7704749946690769748ull);
}

size_t gen_word_len(size_t max_len)
{
    size_t Q = max_len / 3;
    size_t R = max_len % 3;

    size_t len = 0;
    for (size_t i = 0; i < Q; ++i) {
        len += prng_next(&prng) & 3;
    }
    len += prng_next(&prng) % (R + 1);

    assert(len <= max_len);
    return len;
}

size_t gen_word_len_almost_full(size_t max_len)
{
    if (max_len <= 4) {
        size_t max_cut = max_len / 2;
        return max_len - prng_next(&prng) % (max_cut + 1);
    }
    return max_len - (prng_next(&prng) & 3);
}

void gen_word(char *buf, size_t len)
{
    static const char ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for (size_t i = 0; i < len; ++i) {
        buf[i] = ALPHABET[prng_next(&prng) & 31];
    }
}
