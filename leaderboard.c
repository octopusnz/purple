#include "leaderboard.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

static const char *GetLeaderboardDir(void)
{
    static char dir[512];
    const char *home = getenv("HOME");
    if (home && home[0] != '\0') {
        snprintf(dir, sizeof(dir), "%s/.purple", home);
    } else {
        // Fallback to current directory
        snprintf(dir, sizeof(dir), ".");
    }
    return dir;
}

static const char *GetLeaderboardPath(void)
{
    static char path[512];
    const char *dir = GetLeaderboardDir();
    if (strcmp(dir, ".") == 0) {
        snprintf(path, sizeof(path), "leaderboard.txt");
    } else {
        snprintf(path, sizeof(path), "%s/leaderboard.txt", dir);
    }
    return path;
}

static void EnsureDirExists(void)
{
    const char *dir = GetLeaderboardDir();
    struct stat st;
    if (stat(dir, &st) == 0 && S_ISDIR(st.st_mode)) return;
    // Try to create; ignore errors
    (void)mkdir(dir, 0700);
}

static int CompareEntries(const void *a, const void *b)
{
    const LeaderboardEntry *ea = (const LeaderboardEntry *)a;
    const LeaderboardEntry *eb = (const LeaderboardEntry *)b;
    if (ea->seconds < eb->seconds) return -1;
    if (ea->seconds > eb->seconds) return 1;
    return 0;
}

void LoadLeaderboard(Leaderboard *lb)
{
    if (!lb) return;
    lb->count = 0;
    const char *path = GetLeaderboardPath();
    FILE *fp = fopen(path, "r");
    if (!fp) return;

    char line[256];
    while (fgets(line, sizeof(line), fp) && lb->count < LEADERBOARD_MAX_ENTRIES) {
        // Format: seconds;winner;initials
        char initials[8] = {0};
        char winner = 'P';
        float seconds = 0.0f;
        if (sscanf(line, "%f;%c;%7s", &seconds, &winner, initials) == 3) {
            LeaderboardEntry *e = &lb->entries[lb->count++];
            e->seconds = seconds;
            e->winner = (winner == 'A') ? 'A' : 'P';
            // Initialize and copy up to 3 chars
            e->initials[0] = e->initials[1] = e->initials[2] = '\0';
            e->initials[3] = '\0';
            strncpy(e->initials, initials, 3);
        }
    }
    fclose(fp);

    // Ensure sorted
    if (lb->count > 1) {
        qsort(lb->entries, lb->count, sizeof(LeaderboardEntry), CompareEntries);
    }
}

void SaveLeaderboard(const Leaderboard *lb)
{
    if (!lb) return;
    EnsureDirExists();
    const char *path = GetLeaderboardPath();
    FILE *fp = fopen(path, "w");
    if (!fp) return;

    for (size_t i = 0; i < lb->count && i < LEADERBOARD_MAX_ENTRIES; ++i) {
        const LeaderboardEntry *e = &lb->entries[i];
        fprintf(fp, "%0.3f;%c;%s\n", (double)e->seconds, e->winner, e->initials);
    }
    fclose(fp);
}

static void UppercaseInitials(char *dst3, const char *src)
{
    size_t i = 0;
    for (; i < 3 && src && src[i]; ++i) {
        char c = src[i];
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        dst3[i] = c;
    }
    for (; i < 3; ++i) dst3[i] = ' ';
    dst3[3] = '\0';
}

void AddLeaderboardEntry(Leaderboard *lb, const char *initials, char winner, float seconds)
{
    if (!lb) return;
    LeaderboardEntry e;
    UppercaseInitials(e.initials, initials ? initials : "   ");
    e.winner = (winner == 'A') ? 'A' : 'P';
    e.seconds = seconds;

    // Insert into array
    if (lb->count < LEADERBOARD_MAX_ENTRIES) {
        lb->entries[lb->count++] = e;
    } else {
        // Replace worst if current is better
        size_t worst = 0;
        for (size_t i = 1; i < lb->count; ++i) {
            if (lb->entries[i].seconds > lb->entries[worst].seconds) worst = i;
        }
        if (e.seconds < lb->entries[worst].seconds) {
            lb->entries[worst] = e;
        } else {
            // Not good enough to enter top list
            return;
        }
    }
    // Resort
    if (lb->count > 1) {
        qsort(lb->entries, lb->count, sizeof(LeaderboardEntry), CompareEntries);
    }
}
