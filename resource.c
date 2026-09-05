/* =========================================================================
    Purple
    https://github.com/octopusnz/purple
    Copyright (c) 2026 Jacob Doherty
    SPDX-License-Identifier: MIT
    File: resource.c
    Description: Resource file location and discovery
========================================================================= */

#include "resource.h"
#include <stdio.h>
#include <sys/stat.h>

#define MAX_PATH_LENGTH 512

// Check if a directory exists
static int LocalDirectoryExists(const char *path)
{
    struct stat st;
    return (stat(path, &st) == 0) && S_ISDIR(st.st_mode);
}

// File-scope (not function-local) so ResetResourceDirectoryCacheForTesting
// can clear them from outside FindResourceDirectory.
static char resourcePathCache[MAX_PATH_LENGTH];
static int  resourceDirInitialized = 0;

// Find resources directory by searching parent directories.
// Public API declared in resource.h; also used directly by the fuzz harness.
// The result is cached after the first call so repeated invocations skip the
// stat() scan entirely.
const char* FindResourceDirectory(void)
{
    if (resourceDirInitialized) return resourcePathCache;

    const char *basePaths[] = {
        "./resources",
        "resources",
        "../resources",
        "../../resources",
        "../../../resources",
    };

    for (size_t i = 0; i < sizeof(basePaths) / sizeof(basePaths[0]); i++) {
        if (LocalDirectoryExists(basePaths[i])) {
            snprintf(resourcePathCache, sizeof(resourcePathCache), "%s", basePaths[i]);
            resourceDirInitialized = 1;
            return resourcePathCache;
        }
    }

    // Default fallback
    snprintf(resourcePathCache, sizeof(resourcePathCache), "%s", "./resources");
    resourceDirInitialized = 1;
    return resourcePathCache;
}

void ResetResourceDirectoryCacheForTesting(void)
{
    resourceDirInitialized = 0;
}

// Find a resource file within the resources directory.
// Public API declared in resource.h; also used directly by the fuzz harness.
const char* FindResourceFile(const char *resourceSubpath)
{
    static char fullPath[MAX_PATH_LENGTH];
    if (!resourceSubpath) return "./resources";
    const char *resourceDir = FindResourceDirectory();
    snprintf(fullPath, sizeof(fullPath), "%s/%s", resourceDir, resourceSubpath);
    return fullPath;
}

// Find font file using generic resource lookup
const char* FindFontPath(void)
{
    return FindResourceFile("orbitron/Orbitron-VariableFont_wght.ttf");
}
