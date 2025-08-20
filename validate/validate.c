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

#include "../jjhash_64/jjhash64.h"
#include "../jjhash_64/jjhashx64.h"

#include "../jjhash.h"
#include "../jjhashx.h"

#if TEST_64

typedef uint64_t HASH_TYPE;
# define HASH_TYPE_FMT PRIu64
# define JJ(token)  jjhash64 ## token
# define JJX(token) jjhashx64 ## token
# define JJX_UP(token) JJHASHX64 ## token

#else

typedef uint32_t HASH_TYPE;
# define HASH_TYPE_FMT PRIu32
# define JJ(token)  jjhash ## token
# define JJX(token) jjhashx ## token
# define JJX_UP(token) JJHASHX ## token

#endif

#include <sys/mman.h>
#include <unistd.h>

enum { W = 16 };

enum { MAX_LEN = 256 };
enum { TORTURE = 1024 };

typedef struct {
    char *page;
    size_t page_size;
} Page;

typedef struct {
    const char *buf;
    size_t len;
} Content;

enum {
    FLAG_OFFSET_FROM_END = 1 << 0,
    FLAG_ZERO_TERMINATE  = 1 << 1,
};

static inline char *copy_by_offset(
    Page page,
    Content content,
    size_t offset,
    int flags)
{
    memset(page.page, 0xff, page.page_size);

    size_t write_len = content.len;
    if (flags & FLAG_ZERO_TERMINATE) {
        ++write_len;
    }

    if (flags & FLAG_OFFSET_FROM_END) {
        offset = page.page_size - offset - write_len;
        if (flags & FLAG_ZERO_TERMINATE) {
            offset &= (page.page_size - 1);
        }
    }

    char *dst = page.page + offset;
    if (content.len) {
        memcpy(dst, content.buf, content.len);
    }
    if (flags & FLAG_ZERO_TERMINATE) {
        dst[content.len] = '\0';
    }

    return dst;
}

static inline HASH_TYPE do_hash(const char *buf, size_t len, bool zero_terminated)
{
    if (zero_terminated) {
        return JJ(_s)(buf);
    } else {
        return JJ(_b)(buf, len);
    }
}

static HASH_TYPE test_content(
    Page page,
    Content content)
{
    assert(page.page_size >= W);
    assert(content.len < page.page_size - W);

    HASH_TYPE true_hash = 0;
    bool first = true;
    size_t true_hash_i = -1;
    int true_hash_flags = -1;

    for (size_t i = 0; i < W; ++i) {
        for (int flags = 0; flags < 4; ++flags) {
            const char *ptr = copy_by_offset(page, content, i, flags);
            HASH_TYPE hash = do_hash(ptr, content.len, !!(flags & FLAG_ZERO_TERMINATE));
            if (first) {
                true_hash = hash;
                first = false;
                true_hash_i = i;
                true_hash_flags = flags;
            } else {
                if (hash != true_hash) {
                    fprintf(stderr, "Hash mismatch:\n");
                    fprintf(stderr, "Content: '%.*s'\n", (int) content.len, content.buf);
                    fprintf(stderr, "Last hash:     %" HASH_TYPE_FMT ", from i=%zu, flags=%d\n", true_hash, true_hash_i, true_hash_flags);
                    fprintf(stderr, "Current hash': %" HASH_TYPE_FMT ", from i=%zu, flags=%d\n", hash, i, flags);
                    abort();
                }
            }
        }
    }

    return true_hash;
}

static struct JJX(_state) hash_state_of_concat_b(const char *s, size_t na, struct JJX(_state) state_a, size_t ntotal)
{
    size_t ntail = JJX_UP(_UNDO_TAIL_SIZE)(na);
    size_t boundary = JJX_UP(_UNDO_TRUNCATE_TAIL)(na);

    struct JJX(_state) new_state = JJX(_undo)(state_a, s + boundary, ntail);
    return JJX(_b_continue)(new_state, s + boundary, ntotal - boundary);
}

static struct JJX(_state) hash_state_of_concat_s(const char *s, size_t na, struct JJX(_state) state_a)
{
    size_t ntail = JJX_UP(_UNDO_TAIL_SIZE)(na);
    size_t boundary = JJX_UP(_UNDO_TRUNCATE_TAIL)(na);

    struct JJX(_state) new_state = JJX(_undo)(state_a, s + boundary, ntail);

    return JJX(_s_continue)(new_state, s + boundary);
}

static HASH_TYPE test_content_undo(
    Page page,
    Content content,
    bool s_mode)
{
    const char *ptr = copy_by_offset(
        page, content, 0,
        FLAG_OFFSET_FROM_END | (s_mode ? FLAG_ZERO_TERMINATE : 0));

    size_t boundary = content.len / 2;

    struct JJX(_state) state_A = JJX(_b_begin)(ptr, boundary);
    struct JJX(_state) state_AB = (s_mode
        ? hash_state_of_concat_s(ptr, boundary, state_A)
        : hash_state_of_concat_b(ptr, boundary, state_A, content.len)
    );

    HASH_TYPE hash_straight = (s_mode
        ? JJX(_s)(ptr)
        : JJX(_b)(ptr, content.len)
    );
    HASH_TYPE hash_undo = JJX(_finalize_state)(state_AB);

    if (hash_straight != hash_undo) {
        fprintf(stderr, "Hash mismatch (undo, mode %c):\n", s_mode ? 's' : 'b');
#define PAIR(ptr, len) (int) (len), (ptr)
        fprintf(
            stderr, "Content: '%.*s' + '%.*s'\n",
            PAIR(ptr, boundary),
            PAIR(ptr + boundary, content.len - boundary));
#undef PAIR
        fprintf(stderr, "Hash (straight): %" HASH_TYPE_FMT "\n", hash_straight);
        fprintf(stderr, "Hash (undo):     %" HASH_TYPE_FMT "\n", hash_undo);
        abort();
    }

    return hash_straight;
}

static HASH_TYPE test_on_string_of_length(
    Page page,
    size_t len,
    char *buf)
{
    gen_word(buf, len);
    Content content = {.buf = buf, .len = len};
    HASH_TYPE r1 = test_content(page, content);
    HASH_TYPE r2 = test_content_undo(page, content, false);
    assert(r2 == r1);
    HASH_TYPE r3 = test_content_undo(page, content, true);
    assert(r3 == r1);
    return r1;
}

static Page alloc_page_or_die(void)
{
#ifdef _SC_PAGESIZE
    long page_size = sysconf(_SC_PAGESIZE);
#else
    long page_size = sysconf(_SC_PAGE_SIZE);
#endif
    if (page_size < 0) {
        perror("sysconf (page size)");
        abort();
    }

    fprintf(stderr, "Page size: %ld\n", page_size);

    void *ptr = mmap(
        NULL,
        page_size * 2u,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        abort();
    }

    if (mprotect(
        ((char *) ptr) + page_size,
        page_size,
        PROT_NONE) < 0)
    {
        perror("mprotect");
        abort();
    }

    return (Page) {
        .page = ptr,
        .page_size = (size_t) page_size,
    };
}

static void print_usage_and_exit(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    fprintf(stderr, "USAGE: validate [--test-sigsegv]\n");
    exit(2);
}

int main(int argc, char **argv)
{
    bool test_sigsegv = false;

    if (argc > 2) {
        print_usage_and_exit("Wrong number of arguments.");

    } else if (argc == 2) {
        if (strcmp(argv[1], "--test-sigsegv") == 0) {
            test_sigsegv = true;
        } else {
            print_usage_and_exit("Unknown argument.");
        }
    }

    Page page = alloc_page_or_die();

    if (test_sigsegv) {
#if defined(__x86_64__)
        asm volatile (
            "testb $1, (%[ptr])\n"
            : /*no outputs*/
            : [ptr] "r" (page.page + page.page_size)
            : "cc", "memory"
        );
        fprintf(stderr, "We didn't segfault!\n");
        return 1;
#else
        fprintf(stderr, "--test-sigsegv is only supported on x86-64.\n");
        abort();
#endif
    }

    gen_word_global_init();

    HASH_TYPE xored_hashes = 0;
    char buf[MAX_LEN];

    for (size_t i = 0; i <= MAX_LEN; ++i) {
        fprintf(stderr, "Testing on strings of length %zu\n", i);
        for (int j = 0; j < TORTURE; ++j) {
            xored_hashes ^= test_on_string_of_length(page, i, buf);
        }
    }

    fprintf(stderr, "OK, xored_hashes=%" HASH_TYPE_FMT "\n", xored_hashes);
}
