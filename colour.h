#ifndef COLOUR_H
#define COLOUR_H

#include <raylib.h>

typedef struct {
    Color colour;
    const char* name;
} ColourOption;

// Get next colour index (with wrapping)
int GetNextColourIndex(int currentIndex, int totalColours);

// Get previous colour index (with wrapping)
int GetPreviousColourIndex(int currentIndex, int totalColours);

#endif // COLOUR_H
