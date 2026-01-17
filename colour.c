#include "colour.h"

int GetNextColourIndex(int currentIndex, int totalColours) {
    if (totalColours <= 0) return 0;
    return (currentIndex + 1) % totalColours;
}

int GetPreviousColourIndex(int currentIndex, int totalColours) {
    if (totalColours <= 0) return 0;
    return (currentIndex - 1 + totalColours) % totalColours;
}
