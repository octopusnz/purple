/* =========================================================================
    Purple
    https://github.com/octopusnz/purple
    Copyright (c) 2026 Jacob Doherty
    SPDX-License-Identifier: MIT
    File: leaderboard.h
    Description: Leaderboard management (load, save, add entries)
========================================================================= */

#ifndef LEADERBOARD_H
#define LEADERBOARD_H

#include <stddef.h>

#define LEADERBOARD_MAX_ENTRIES 10

typedef struct {
    char initials[4];
    char winner;   // 'P' for player, 'A' for AI
    float seconds; // time to win
} LeaderboardEntry;

typedef struct {
    LeaderboardEntry entries[LEADERBOARD_MAX_ENTRIES];
    size_t count;
} Leaderboard;

// Load leaderboard from persistent storage
void LoadLeaderboard(Leaderboard *lb);

// Save leaderboard to persistent storage
void SaveLeaderboard(const Leaderboard *lb);

// Add an entry (keeps only fastest LEADERBOARD_MAX_ENTRIES, sorted ascending by time)
void AddLeaderboardEntry(Leaderboard *lb, const char *initials, char winner, float seconds);

#endif // LEADERBOARD_H
