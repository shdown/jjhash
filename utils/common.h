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

#pragma once

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#define in_header       static inline __attribute__((unused))
#define likely(E)       __builtin_expect(!!(E), 1)
#define unlikely(E)     __builtin_expect(!!(E), 0)
#define array_size(A)   (sizeof(A) / sizeof((A)[0]))

in_header __attribute__((noreturn))
void die_out_of_memory(void)
{
    fputs("Out of memory.\n", stderr);
    abort();
}

void *realloc_or_die(void *p, size_t n, size_t m);

void *calloc_or_die(size_t n, size_t m);

void *malloc_or_die(size_t n, size_t m);

void *x2realloc_or_die(void *p, size_t *n, size_t m);

void *memdup_or_die(const void *p, size_t n);

char *strdup_or_die(const char *s);

__attribute__((format(printf, 1, 0)))
char *allocvf_or_die(const char *fmt, va_list vl);

__attribute__((format(printf, 1, 2)))
char *allocf_or_die(const char *fmt, ...);
