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

#ifndef JJHASHX64_INCLUDED__
#define JJHASHX64_INCLUDED__

#include <stdint.h>
#include <stddef.h>

#ifndef JJHASHX64_ATTRS_SMALL
# define JJHASHX64_ATTRS_SMALL inline
#endif
#ifndef JJHASHX64_ATTRS_BIG
# define JJHASHX64_ATTRS_BIG inline
#endif

#define JJHASHX64_PRIME UINT64_C(2752750471)
#define JJHASHX64_PRIME_MODINV UINT64_C(5082482002835059255)
#define JJHASHX64_ACCUM_INIT UINT64_C(0x100000000)

#define JJHASHX64_ACCUM_FEED(a, v) do { a ^= (v); a *= JJHASHX64_PRIME; } while (0)
#define JJHASHX64_ACCUM_FINALIZE(a) do { a^= a >> 16; a ^= a >> 8; } while (0)

#define JJHASHX64_RETURN_STATE(a) do { struct jjhashx64_state r_ = {a}; return r_; } while (0)

// We wrap state into a struct so that it be harder to do the wrong thing (e.g. treat is as a hash).
// Use jjhashx64_finalize_state to convert a state into a hash.
struct jjhashx64_state {
    uint64_t the_state;
};

static JJHASHX64_ATTRS_SMALL uint64_t jjhashx64_finalize_state(struct jjhashx64_state state)
{
    uint64_t a = state.the_state;
    JJHASHX64_ACCUM_FINALIZE(a);
    return a;
}

static JJHASHX64_ATTRS_BIG struct jjhashx64_state jjhashx64_b_continue(struct jjhashx64_state state, const char *s, size_t ns)
{
    uint64_t a = state.the_state;

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

            JJHASHX64_ACCUM_FEED(a, v);
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
        JJHASHX64_ACCUM_FEED(a, v);
    }

    JJHASHX64_RETURN_STATE(a);
}

static JJHASHX64_ATTRS_SMALL struct jjhashx64_state jjhashx64_b_begin(const char *s, size_t ns)
{
    struct jjhashx64_state state = {JJHASHX64_ACCUM_INIT};
    return jjhashx64_b_continue(state, s, ns);
}

static JJHASHX64_ATTRS_SMALL uint64_t jjhashx64_b(const char *s, size_t ns)
{
    return jjhashx64_finalize_state(jjhashx64_b_begin(s, ns));
}

static JJHASHX64_ATTRS_BIG struct jjhashx64_state jjhashx64_s_continue(struct jjhashx64_state state, const char *s)
{
    uint64_t a = state.the_state;

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

        JJHASHX64_ACCUM_FEED(a, v);

        s += 4;
    }

rem_3:
    c0 |= (c2 << 16);
rem_2:
    c0 |= (c1 << 8);
rem_1:
    JJHASHX64_ACCUM_FEED(a, c0);
rem_0:
    JJHASHX64_RETURN_STATE(a);
}

static JJHASHX64_ATTRS_SMALL struct jjhashx64_state jjhashx64_s_begin(const char *s)
{
    struct jjhashx64_state state = {JJHASHX64_ACCUM_INIT};
    return jjhashx64_s_continue(state, s);
}

static JJHASHX64_ATTRS_SMALL uint64_t jjhashx64_s(const char *s)
{
    return jjhashx64_finalize_state(jjhashx64_s_begin(s));
}

// The intended use is as follows:
//
// Calculate hash state of concatenation of "prefix" (s ... s+na) with "suffix" (s+na ... s+ntotal).
// state_a is hash sate of the "prefix" (s ... s+na).
//
// struct JJHASHX64_state hash_state_of_concat_b(const char *s, size_t na, struct JJHASHX64_state state_a, size_t ntotal)
// {
//     size_t ntail = JJHASHX64_UNDO_TAIL_SIZE(na);
//     size_t boundary = JJHASHX64_UNDO_TRUNCATE_TAIL(na);
//
//     struct JJHASHX64_state new_state = JJHASHX64_undo(state_a, s + boundary, ntail);
//     return JJHASHX64_b_continue(new_state, s + boundary, ntotal - boundary);
// }
//
// Calculate hash state of concatenation of "prefix" (s ... s+na) with "suffix" (s+na ... s+strlen(s)-na).
// state_a is hash state of the "prefix" (s ... s+na).
//
// struct JJHASHX64_state hash_state_of_concat_s(const char *s, size_t na, struct JJHASHX64_state state_a)
// {
//     size_t ntail = JJHASHX64_UNDO_TAIL_SIZE(na);
//     size_t boundary = JJHASHX64_UNDO_TRUNCATE_TAIL(na);
//
//     struct JJHASHX64_state new_state = JJHASHX64_undo(state_a, s + boundary, ntail);
//
//     return JJHASHX64_s_continue(new_state, s + boundary);
// }

#define JJHASHX64_UNDO_STEP 4

#define JJHASHX64_UNDO_TRUNCATE_TAIL(n) ((n) & ~(JJHASHX64_UNDO_STEP - 1))

#define JJHASHX64_UNDO_TAIL_SIZE(n) ((n) & (JJHASHX64_UNDO_STEP - 1))

static JJHASHX64_ATTRS_BIG struct jjhashx64_state jjhashx64_undo(struct jjhashx64_state state, const char *tail, size_t ntail)
{
    uint64_t a = state.the_state;

    if (ntail) {
        a *= JJHASHX64_PRIME_MODINV;

        uint32_t v = (uint8_t) tail[0];
        if (ntail > 1) {
            uint32_t c1 = (uint8_t) tail[1];
            v |= (c1 << 8);
            if (ntail > 2) {
                uint32_t c2 = (uint8_t) tail[2];
                v |= (c2 << 16);
            }
        }
        a ^= v;
    }

    JJHASHX64_RETURN_STATE(a);
}

#endif // JJHASHX64_INCLUDED__
