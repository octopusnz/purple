/* =========================================================================
    Purple - Fuzz Testing
    https://github.com/octopusnz/purple
    Copyright (c) 2026 Jacob Doherty
    SPDX-License-Identifier: MIT
    See LICENSE.txt for 3rd party library and other resource licenses.
    File: fuzz_initials_roundtrip.c
    Description: Fuzz testing for UppercaseInitials correctness via
                 AddLeaderboardEntry.

    UppercaseInitials is a static function in leaderboard.c that maps
    'a'-'z' → 'A'-'Z' and passes all other characters through unchanged,
    padding the result to exactly 3 chars (with spaces) and null-terminating
    at position 3.  No existing harness verifies this character-level mapping
    invariant; they only check structural invariants (null-termination, count).

    This harness tests the mapping directly by:
      1. Feeding arbitrary 3-byte initials through AddLeaderboardEntry.
      2. Asserting that every 'a'-'z' input byte becomes the corresponding
         'A'-'Z' in the stored entry.
      3. Asserting that every non-lowercase, non-NUL byte is stored as-is.
      4. Asserting that positions after a NUL in the source are padded with ' '.
      5. Asserting that initials[3] is always '\0'.
      6. Asserting that lb.count never exceeds LEADERBOARD_MAX_ENTRIES.

    Input layout (minimum 4 bytes):
      [0..2]  raw bytes  initials source (all 256 values exercised per position)
      [3]     uint8      winner byte (raw — normalization exercised)
      [4..7]  float      seconds (optional; defaults to 1.0f)
      [8..]   additional entries (8-byte chunks: initials[3] | winner | seconds)
========================================================================= */

#include <stdint.h>
#include <string.h>
#include <math.h>
#include "../leaderboard.h"

/* Predict what UppercaseInitials will store for src[i], given prior bytes.
 * Returns the expected character at output position i, or -1 if we can't
 * determine it (e.g., position is after the first NUL in src and we need
 * to check for space-padding instead).
 */
static char ExpectedInitial(const char *src, int i)
{
    /* Scan for NUL before position i */
    for (int j = 0; j < i; ++j) {
        if (src[j] == '\0') {
            /* Position i is beyond the source NUL → padded with ' ' */
            return ' ';
        }
    }
    char c = src[i];
    if (c == '\0') {
        /* This position IS the NUL terminator → padded with ' ' */
        return ' ';
    }
    if (c >= 'a' && c <= 'z') {
        return (char)(c - 'a' + 'A');
    }
    return c;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 4) return 0;

    /* Build a null-terminated source string from the first 3 input bytes.
     * We keep the raw bytes (including embedded NULs) so the fuzzer can
     * explore the padding path (src shorter than 3 chars).
     */
    char initials_src[4];
    initials_src[0] = (char)data[0];
    initials_src[1] = (char)data[1];
    initials_src[2] = (char)data[2];
    initials_src[3] = '\0';

    char winner = (char)data[3];

    float seconds = 1.0f;
    if (size >= 8) {
        memcpy(&seconds, data + 4, sizeof(float));
        /* Reject non-finite and negative seconds so the entry is always added */
        if (!isfinite(seconds) || seconds < 0.0f) seconds = 1.0f;
    }

    Leaderboard lb = {0};
    AddLeaderboardEntry(&lb, initials_src, winner, seconds);

    /* ----- Invariant: count must not exceed LEADERBOARD_MAX_ENTRIES ----- */
    if (lb.count > LEADERBOARD_MAX_ENTRIES) {
        __builtin_trap();
    }

    /* The entry must have been added (fresh leaderboard, valid seconds) */
    if (lb.count != 1) {
        /* AddLeaderboardEntry failed to add a valid entry */
        __builtin_trap();
    }

    const LeaderboardEntry *e = &lb.entries[0];

    /* ----- Invariant: initials[3] is always '\0' ----- */
    if (e->initials[3] != '\0') {
        __builtin_trap();
    }

    /* ----- Invariant: character mapping correctness (UppercaseInitials) -----
     * For each output position 0..2, verify the expected transformation.
     */
    for (int i = 0; i < 3; ++i) {
        char expected = ExpectedInitial(initials_src, i);
        if (e->initials[i] != expected) {
            /* UppercaseInitials produced wrong character */
            __builtin_trap();
        }
    }

    /* ----- Invariant: winner is normalised to 'P' or 'A' ----- */
    if (e->winner != 'P' && e->winner != 'A') {
        __builtin_trap();
    }

    /* ----- Stress test: add many more entries with adversarial initials -----
     * Use remaining input bytes in 8-byte chunks.  This exercises the
     * replacement/sort path together with exotic initials characters.
     */
    size_t offset = 8;
    while (offset + 8 <= size) {
        char bulk_initials[4];
        bulk_initials[0] = (char)data[offset + 0];
        bulk_initials[1] = (char)data[offset + 1];
        bulk_initials[2] = (char)data[offset + 2];
        bulk_initials[3] = '\0';

        char bulk_winner = (char)data[offset + 3];
        float bulk_seconds;
        memcpy(&bulk_seconds, data + offset + 4, sizeof(float));

        AddLeaderboardEntry(&lb, bulk_initials, bulk_winner, bulk_seconds);
        offset += 8;
    }

    /* ----- Final structural invariants after bulk insertions ----- */
    if (lb.count > LEADERBOARD_MAX_ENTRIES) {
        __builtin_trap();
    }

    for (size_t i = 0; i < lb.count; ++i) {
        /* Null-termination */
        if (lb.entries[i].initials[3] != '\0') {
            __builtin_trap();
        }

        /* Each stored initial that was lowercase 'a'-'z' must be uppercased.
         * We can only check this for the first entry whose src we know; for
         * bulk entries we just verify the stored char is not a raw lowercase
         * letter (UppercaseInitials must have transformed it).
         */
        for (int j = 0; j < 3; ++j) {
            char c = lb.entries[i].initials[j];
            if (c >= 'a' && c <= 'z') {
                /* Lowercase survived UppercaseInitials — mapping is broken */
                __builtin_trap();
            }
        }

        /* Winner must be normalised */
        if (lb.entries[i].winner != 'P' && lb.entries[i].winner != 'A') {
            __builtin_trap();
        }

        /* Seconds must be finite and non-negative */
        if (!isfinite(lb.entries[i].seconds) || lb.entries[i].seconds < 0.0f) {
            __builtin_trap();
        }
    }

    /* Sorting invariant */
    for (size_t i = 1; i < lb.count; ++i) {
        if (lb.entries[i].seconds < lb.entries[i - 1].seconds) {
            __builtin_trap();
        }
    }

    return 0;
}
