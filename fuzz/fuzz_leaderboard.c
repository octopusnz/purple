/* =========================================================================
    Purple - Fuzz Testing
    https://github.com/octopusnz/purple
    Copyright (c) 2026 Jacob Doherty
    SPDX-License-Identifier: MIT
    See LICENSE.txt for 3rd party library and other resource licenses.
    File: fuzz_leaderboard.c
    Description: Coverage-guided fuzz testing for leaderboard sorting and persistence
========================================================================= */

#include <stdint.h>
#include <string.h>
#include <math.h>
#include "../leaderboard.h"

/* Fuzz target: test leaderboard operations with random entries
 * Tests sorting, boundary conditions, and string handling
 */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 8) return 0;  // Need minimum data

    Leaderboard lb = {0};
    
    /* Parse fuzzer input to create leaderboard entries */
    size_t offset = 0;
    while (offset + 8 <= size) {
        /* Extract initials (3 bytes) */
        char initials[4] = {0};
        initials[0] = (char)data[offset];
        initials[1] = (char)data[offset + 1];
        initials[2] = (char)data[offset + 2];
        initials[3] = '\0';
        
        /* Extract winner type (1 byte) — use raw byte to exercise the
         * 'A'/'P' normalization in AddLeaderboardEntry for all 256 values.
         */
        char winner = (char)data[offset + 3];
        
        /* Extract time (4 bytes as float) */
        float seconds;
        memcpy(&seconds, data + offset + 4, sizeof(float));
        
        /* Clamp seconds to reasonable range */
        if (seconds < 0.0f) seconds = -seconds;
        if (seconds > 10000.0f) seconds = 10000.0f;
        
        /* Add entry to leaderboard */
        AddLeaderboardEntry(&lb, initials, winner, seconds);

        offset += 8;
    }

    /* Verify that NaN / Inf / negative seconds are silently rejected */
    size_t pre_count = lb.count;
    AddLeaderboardEntry(&lb, "NAN", 'P', NAN);
    AddLeaderboardEntry(&lb, "INF", 'P', INFINITY);
    AddLeaderboardEntry(&lb, "NEG", 'P', -1.0f);
    if (lb.count > pre_count) {
        /* These invalid entries must never increase count */
        __builtin_trap();
    }

    /* Verify leaderboard invariants */
    if (lb.count > LEADERBOARD_MAX_ENTRIES) {
        /* Leaderboard exceeded max entries */
        __builtin_trap();
    }
    
    /* Verify entries are sorted (ascending by time) */
    for (size_t i = 1; i < lb.count; ++i) {
        if (lb.entries[i].seconds < lb.entries[i-1].seconds) {
            /* Sorting invariant violated */
            __builtin_trap();
        }
    }
    
    /* Test that all initials are null-terminated */
    for (size_t i = 0; i < lb.count; ++i) {
        if (lb.entries[i].initials[3] != '\0') {
            /* Null termination missing */
            __builtin_trap();
        }
    }

    /* Equal-time stability: fill a fresh leaderboard with LEADERBOARD_MAX_ENTRIES
     * entries that all share the same time, then add one more.  The sort must
     * still satisfy the ascending-order invariant and must not exceed the cap.
     *
     * Uses a fuzz-derived time so libFuzzer can explore different float values
     * including subnormals near zero.
     */
    {
        Leaderboard eqlb = {0};
        float eqTime;
        if (size >= 4) {
            memcpy(&eqTime, data, sizeof(float));
            if (!isfinite(eqTime) || eqTime < 0.0f) eqTime = 42.0f;
        } else {
            eqTime = 42.0f;
        }

        for (int e = 0; e < LEADERBOARD_MAX_ENTRIES + 2; ++e) {
            AddLeaderboardEntry(&eqlb, "EQL", 'P', eqTime);
        }

        if (eqlb.count > LEADERBOARD_MAX_ENTRIES) __builtin_trap();

        for (size_t i = 1; i < eqlb.count; ++i) {
            if (eqlb.entries[i].seconds < eqlb.entries[i-1].seconds) {
                __builtin_trap();
            }
        }
    }

    return 0;
}
