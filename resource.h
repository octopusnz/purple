#ifndef RESOURCE_H
#define RESOURCE_H

// Find a resource file within the resources directory
// Searches multiple possible locations for the resources folder
// Returns a path to the resource file (caller should not free)
const char* FindResourceFile(const char *resourceSubpath);

// Find the font file specifically
// Returns a path to the Orbitron font (caller should not free)
const char* FindFontPath(void);

// Find the resources directory by searching parent directories
// Returns a path to the resources directory (caller should not free)
const char* FindResourceDirectory(void);

#endif // RESOURCE_H
