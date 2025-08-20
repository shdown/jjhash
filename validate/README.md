# Description

We check the following things:
  1. `jjhash_s` (function to hash a null-terminated string) and `jjhash_b` (function to hash a pointer-and-length string) agree on the hash of the same string;
  2. calculating the hash of concatenation from the previous state and the new string works as expected;
  3. the functions do not make unsafe reads past their data (this may lead to a segmentation fault in a real-world program):
we check it by placing the string just before a “poisoned page” (first we allocate two normal pages with `mmap()`, then poison the second page with `mprotect(..., prot=PROT_NONE)`);
  4. all the properties above are invariant over the alignment of the pointer to the beginning of the string.

# Reproduction

Unlike the hash itself, validation code requires a GNU C-compatible compiler, a somewhat POSIX-compliant OS with support for `MAP_ANONYMOUS` flag to `mmap()` (Linux, BSD or Mac OS would do), and bash.

To compile and run the validation program (for both `jjhash` and `jjhash_64`), run `./build_and_validate.sh`.
