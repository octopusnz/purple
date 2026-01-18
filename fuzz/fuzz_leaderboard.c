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
        
        /* Extract winner type (1 byte) */
        char winner = (data[offset + 3] % 2 == 0) ? 'P' : 'A';
        
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
    
    return 0;
}
