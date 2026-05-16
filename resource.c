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

// Find resources directory by searching parent directories.
// Public API declared in resource.h; also used directly by the fuzz harness.
// The result is cached after the first call so repeated invocations skip the
// stat() scan entirely.
const char* FindResourceDirectory(void)
{
    static char resourcePath[MAX_PATH_LENGTH];
    static int  initialized = 0;

    if (initialized) return resourcePath;

    const char *basePaths[] = {
        "./resources",
        "resources",
        "../resources",
        "../../resources",
        "../../../resources",
    };

    for (size_t i = 0; i < sizeof(basePaths) / sizeof(basePaths[0]); i++) {
        if (LocalDirectoryExists(basePaths[i])) {
            snprintf(resourcePath, sizeof(resourcePath), "%s", basePaths[i]);
            initialized = 1;
            return resourcePath;
        }
    }

    // Default fallback
    snprintf(resourcePath, sizeof(resourcePath), "%s", "./resources");
    initialized = 1;
    return resourcePath;
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
