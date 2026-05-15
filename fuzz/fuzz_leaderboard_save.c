/* =========================================================================
    Purple - Fuzz Testing
    https://github.com/octopusnz/purple
    Copyright (c) 2026 Jacob Doherty
    SPDX-License-Identifier: MIT
    See LICENSE.txt for 3rd party library and other resource licenses.
    File: fuzz_leaderboard_save.c
    Description: Coverage-guided fuzz testing for SaveLeaderboard and the
                 save→load round-trip.

    SaveLeaderboard is exercised with random Leaderboard structs built via
    AddLeaderboardEntry, then LoadLeaderboard is called on the written file
    to verify the round-trip.  Key paths exercised that no other fuzzer hits:

      - EnsureDirExists / GetLeaderboardDir / GetLeaderboardPath code paths
      - fprintf formatting of seconds/winner/initials with exotic values
      - Raw winner bytes (not just 'P'/'A') to exercise normalization
      - NULL initials fallback ("   ")
      - NaN / Inf / negative seconds rejection in AddLeaderboardEntry

    Invariants checked after LoadLeaderboard:
      1. lb2.count <= LEADERBOARD_MAX_ENTRIES
      2. lb2.count <= lb.count  (save→load cannot invent new entries)
      3. Every initials[3] == '\0'
      4. Every seconds is finite and >= 0
      5. Every winner is 'P' or 'A'
      6. Entries are sorted ascending by seconds
      7. NaN / Inf / negative seconds are silently rejected by AddLeaderboardEntry
========================================================================= */

/* Required for setenv, mkdir (POSIX.1-2001) */
#define _POSIX_C_SOURCE 200112L

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include "../leaderboard.h"

/* Per-process temp HOME set up once in LLVMFuzzerInitialize */
static char s_home_dir[64];
static char s_purple_dir[80];

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;

    snprintf(s_home_dir,   sizeof(s_home_dir),  "/tmp/fuzz_save_%d",          (int)getpid());
    snprintf(s_purple_dir, sizeof(s_purple_dir), "/tmp/fuzz_save_%d/.purple",  (int)getpid());

    (void)mkdir(s_home_dir,   0700);
    (void)mkdir(s_purple_dir, 0700);

    setenv("HOME", s_home_dir, 1);
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 8) return 0;

    Leaderboard lb = {0};

    /* Build leaderboard from fuzzer data.
     * Each 8-byte chunk encodes: initials[0..2] | winner | seconds (float).
     * The winner byte is passed raw to exercise the 'A'/'P' normalization in
     * AddLeaderboardEntry for every possible byte value.
     */
    size_t offset = 0;
    while (offset + 8 <= size) {
        char initials[4];
        initials[0] = (char)data[offset + 0];
        initials[1] = (char)data[offset + 1];
        initials[2] = (char)data[offset + 2];
        initials[3] = '\0';

        char winner = (char)data[offset + 3];   /* raw byte — normalization exercised */

        float seconds;
        memcpy(&seconds, data + offset + 4, sizeof(float));

        AddLeaderboardEntry(&lb, initials, winner, seconds);
        offset += 8;
    }

    /* Exercise the NULL initials fallback path ("   ") */
    AddLeaderboardEntry(&lb, NULL, 'P', 1.0f);

    /* ----- Invariant 7: NaN / Inf / negative seconds must be rejected ----- */
    size_t pre_count = lb.count;
    AddLeaderboardEntry(&lb, "NAN", 'P', NAN);
    AddLeaderboardEntry(&lb, "INF", 'P', INFINITY);
    AddLeaderboardEntry(&lb, "NEG", 'P', -1.0f);
    if (lb.count > pre_count) {
        /* NaN / Inf / negative entry must never increase count */
        __builtin_trap();
    }

    /* Save to disk */
    SaveLeaderboard(&lb);

    /* Reload and verify round-trip invariants */
    Leaderboard lb2 = {0};
    LoadLeaderboard(&lb2);

    /* ----- Invariant 1: count must not exceed maximum ----- */
    if (lb2.count > LEADERBOARD_MAX_ENTRIES) {
        __builtin_trap();
    }

    /* ----- Invariant 2: load cannot produce more entries than were saved -----
     * Entries with special characters in initials (newlines, semicolons, etc.)
     * may fail to round-trip, so lb2.count <= lb.count is the safe bound.
     */
    if (lb2.count > lb.count) {
        __builtin_trap();
    }

    for (size_t i = 0; i < lb2.count; ++i) {
        const LeaderboardEntry *e = &lb2.entries[i];

        /* ----- Invariant 3: initials must be null-terminated ----- */
        if (e->initials[3] != '\0') {
            __builtin_trap();
        }

        /* ----- Invariant 4: seconds must be finite and non-negative ----- */
        float s = e->seconds;
        if (s != s) {           /* NaN self-inequality test */
            __builtin_trap();
        }
        if (!isfinite(s)) {
            __builtin_trap();
        }
        if (s < 0.0f) {
            __builtin_trap();
        }

        /* ----- Invariant 5: winner must be 'P' or 'A' ----- */
        if (e->winner != 'P' && e->winner != 'A') {
            __builtin_trap();
        }
    }

    /* ----- Invariant 6: entries must be sorted ascending by seconds ----- */
    for (size_t i = 1; i < lb2.count; ++i) {
        if (lb2.entries[i].seconds < lb2.entries[i - 1].seconds) {
            __builtin_trap();
        }
    }

    return 0;
}
