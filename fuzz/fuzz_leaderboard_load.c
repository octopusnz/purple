/* =========================================================================
    Purple - Fuzz Testing
    https://github.com/octopusnz/purple
    Copyright (c) 2026 Jacob Doherty
    SPDX-License-Identifier: MIT
    See LICENSE.txt for 3rd party library and other resource licenses.
    File: fuzz_leaderboard_load.c
    Description: Coverage-guided fuzz testing for LoadLeaderboard file parsing

    Exercises the sscanf-based file parser in leaderboard.c with arbitrary
    byte sequences:
      - NaN / Inf in the seconds field (sscanf parses these as valid floats
        on POSIX; qsort with a NaN comparator has non-transitive behaviour)
      - Lines longer than the 256-byte fgets buffer (truncation path)
      - Binary content and embedded null bytes
      - More than LEADERBOARD_MAX_ENTRIES lines (overflow guard)
      - winner field values other than 'A' or 'P'

    Invariants asserted after each LoadLeaderboard call:
      1. lb.count <= LEADERBOARD_MAX_ENTRIES
      2. Every initials[3] == '\0'
      3. Every seconds value is finite and >= 0
      4. Every winner is 'P' or 'A'
      5. Entries are sorted ascending by seconds
========================================================================= */

/* Required for setenv, mkdir (POSIX.1-2001) */
#define _POSIX_C_SOURCE 200112L

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include "../leaderboard.h"

/* Per-process temp paths set up once in LLVMFuzzerInitialize */
static char s_home_dir[64];
static char s_purple_dir[80];
static char s_lb_path[100];

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;

    /* Create a unique temp directory rooted at /tmp so that HOME can be
     * overridden without touching the real user home directory.
     */
    snprintf(s_home_dir,   sizeof(s_home_dir),   "/tmp/fuzz_lb_%d",                  (int)getpid());
    snprintf(s_purple_dir, sizeof(s_purple_dir),  "/tmp/fuzz_lb_%d/.purple",          (int)getpid());
    snprintf(s_lb_path,    sizeof(s_lb_path),     "/tmp/fuzz_lb_%d/.purple/leaderboard.txt", (int)getpid());

    (void)mkdir(s_home_dir,   0700);
    (void)mkdir(s_purple_dir, 0700);

    /* Point LoadLeaderboard at the temp directory */
    setenv("HOME", s_home_dir, 1);

    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Write fuzzer bytes verbatim as the leaderboard file content.
     * This lets libFuzzer explore binary garbage, embedded NULs, lines that
     * exceed the 256-byte fgets buffer, and NaN/Inf float strings.
     */
    FILE *fp = fopen(s_lb_path, "wb");
    if (!fp) return 0;
    fwrite(data, 1, size, fp);
    fclose(fp);

    Leaderboard lb = {0};
    LoadLeaderboard(&lb);

    /* ----- Invariant 1: count must not exceed the maximum ----- */
    if (lb.count > LEADERBOARD_MAX_ENTRIES) {
        __builtin_trap();
    }

    for (size_t i = 0; i < lb.count; ++i) {
        const LeaderboardEntry *e = &lb.entries[i];

        /* ----- Invariant 2: initials must be null-terminated ----- */
        if (e->initials[3] != '\0') {
            __builtin_trap();
        }

        /* ----- Invariant 3: seconds must be finite and non-negative -----
         * sscanf on POSIX parses "nan" and "inf" as valid floats.  If those
         * slip through, qsort's comparator becomes non-transitive (all NaN
         * comparisons return 0), which is undefined behaviour in C.
         */
        float s = e->seconds;
        if (s != s) {           /* NaN self-inequality test */
            __builtin_trap();
        }
        if (!isfinite(s)) {     /* +Inf / -Inf */
            __builtin_trap();
        }
        if (s < 0.0f) {
            __builtin_trap();
        }

        /* ----- Invariant 4: winner must be 'P' or 'A' ----- */
        if (e->winner != 'P' && e->winner != 'A') {
            __builtin_trap();
        }
    }

    /* ----- Invariant 5: entries must be sorted ascending by seconds ----- */
    for (size_t i = 1; i < lb.count; ++i) {
        if (lb.entries[i].seconds < lb.entries[i - 1].seconds) {
            __builtin_trap();
        }
    }

    return 0;
}
