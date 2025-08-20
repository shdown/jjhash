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

#ifndef JJHASH64_INCLUDED__
#define JJHASH64_INCLUDED__

#include <stdint.h>
#include <stddef.h>

#ifndef JJHASH64_ATTRS
# define JJHASH64_ATTRS inline
#endif

#define JJHASH64_PRIME UINT64_C(2752750471)
#define JJHASH64_ACCUM_INIT UINT64_C(0x100000000)

#define JJHASH64_ACCUM_FEED(a, v) do { a ^= (v); a *= JJHASH64_PRIME; } while (0)
#define JJHASH64_ACCUM_FINALIZE(a) do { a ^= a >> 16; a ^= a >> 8; } while (0)

static JJHASH64_ATTRS uint64_t jjhash64_b(const char *s, size_t ns)
{
    uint64_t a = JJHASH64_ACCUM_INIT;

    if (ns >= 4) {
        const char *p = s;
        s += (ns & ~3);
        do {
            // Fetch 4 bytes in little endian; on x86-64, the following 5 lines
            // optimize to a single mov instruction.
            uint32_t c0 = (uint8_t) p[0];
            uint32_t c1 = (uint8_t) p[1];
            uint32_t c2 = (uint8_t) p[2];
            uint32_t c3 = (uint8_t) p[3];
            uint32_t v = c0 | (c1 << 8) | (c2 << 16) | (c3 << 24);

            JJHASH64_ACCUM_FEED(a, v);

            p += 4;
        } while (p != s);
    }

    size_t ntail = ns & 3;
    if (ntail) {
        uint32_t v = (uint8_t) s[0];
        if (ntail > 1) {
            uint32_t c1 = (uint8_t) s[1];
            v |= (c1 << 8);
            if (ntail > 2) {
                uint32_t c2 = (uint8_t) s[2];
                v |= (c2 << 16);
            }
        }
        JJHASH64_ACCUM_FEED(a, v);
    }

    JJHASH64_ACCUM_FINALIZE(a);
    return a;
}

static JJHASH64_ATTRS uint64_t jjhash64_s(const char *s)
{
    uint64_t a = JJHASH64_ACCUM_INIT;

    uint32_t c0;
    uint32_t c1;
    uint32_t c2;
    uint32_t c3;

    for (;;) {
        if (!(c0 = (uint8_t) s[0])) { goto rem_0; }
        if (!(c1 = (uint8_t) s[1])) { goto rem_1; }
        if (!(c2 = (uint8_t) s[2])) { goto rem_2; }
        if (!(c3 = (uint8_t) s[3])) { goto rem_3; }

        uint32_t v = c0 | (c1 << 8) | (c2 << 16) | (c3 << 24);

        JJHASH64_ACCUM_FEED(a, v);

        s += 4;
    }

rem_3:
    c0 |= (c2 << 16);
rem_2:
    c0 |= (c1 << 8);
rem_1:
    JJHASH64_ACCUM_FEED(a, c0);
rem_0:
    JJHASH64_ACCUM_FINALIZE(a);
    return a;
}

#endif // JJHASH64_INCLUDED__
