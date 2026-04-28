/* =========================================================================
    Purple - Fuzz Testing
    https://github.com/octopusnz/purple
    Copyright (c) 2026 Jacob Doherty
    SPDX-License-Identifier: MIT
    See LICENSE.txt for 3rd party library and other resource licenses.
    File: fuzz_resource_path.c
    Description: Coverage-guided fuzz testing for resource path construction

    Exercises FindResourceFile and FindResourceDirectory in resource.c with
    arbitrary subpath strings:
      - Very long subpaths that exceed the 512-byte MAX_PATH_LENGTH buffer
        (snprintf must truncate cleanly, not overflow)
      - Paths containing special characters (spaces, '..', '/', null bytes)
      - Empty string subpath
      - NULL subpath (size == 0 special case) -- FindResourceFile(NULL) passes
        NULL to snprintf's %s argument, which is undefined behaviour on C99;
        this case is tested explicitly so ASAN/UBSan can detect the bug.

    Invariants asserted:
      1. FindResourceFile returns non-NULL for any input
      2. The returned string is null-terminated within MAX_PATH_LENGTH bytes
      3. FindResourceDirectory returns non-NULL
      4. FindFontPath returns non-NULL
========================================================================= */

#include <stdint.h>
#include <string.h>
#include "../resource.h"

/* Must match MAX_PATH_LENGTH in resource.c */
#define FUZZ_MAX_PATH_LEN 512

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* --- Special case: size == 0 exercises FindResourceFile(NULL) ---
     * Passing NULL for the %s argument in snprintf is undefined behaviour
     * per C99.  ASAN / UBSan should detect the violation here, making this
     * a useful regression check for adding a NULL guard to FindResourceFile.
     */
    if (size == 0) {
        FindResourceFile(NULL);
        return 0;
    }

    /* Build a null-terminated copy of the fuzzer data to use as a subpath.
     * Embedded NUL bytes in the fuzzer data are preserved: snprintf will
     * stop copying at the first NUL, exercising the truncation path.
     */
    char subpath[FUZZ_MAX_PATH_LEN + 1];
    size_t copy_len = size < FUZZ_MAX_PATH_LEN ? size : FUZZ_MAX_PATH_LEN;
    memcpy(subpath, data, copy_len);
    subpath[copy_len] = '\0';

    /* ----- Invariant 1: result is non-NULL for any input ----- */
    const char *result = FindResourceFile(subpath);
    if (result == NULL) {
        __builtin_trap();
    }

    /* ----- Invariant 2: result is null-terminated within MAX_PATH_LENGTH -----
     * Walk the returned buffer up to FUZZ_MAX_PATH_LEN + 1 bytes looking for
     * a null terminator.  Reaching the limit means the static buffer in
     * resource.c overflowed (snprintf did not terminate correctly).
     */
    size_t result_len;
    for (result_len = 0; result_len <= FUZZ_MAX_PATH_LEN; ++result_len) {
        if (result[result_len] == '\0') break;
    }
    if (result_len > FUZZ_MAX_PATH_LEN) {
        /* No null terminator found within the expected buffer size */
        __builtin_trap();
    }

    /* ----- Invariant 3: FindResourceDirectory returns non-NULL ----- */
    const char *dir = FindResourceDirectory();
    if (dir == NULL) {
        __builtin_trap();
    }

    /* ----- Invariant 4: FindFontPath returns non-NULL ----- */
    const char *font = FindFontPath();
    if (font == NULL) {
        __builtin_trap();
    }

    return 0;
}
