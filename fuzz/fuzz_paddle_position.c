/* =========================================================================
    Purple - Fuzz Testing
    https://github.com/octopusnz/purple
    Copyright (c) 2026 Jacob Doherty
    SPDX-License-Identifier: MIT
    See LICENSE.txt for 3rd party library and other resource licenses.
    File: fuzz_paddle_position.c
    Description: Coverage-guided fuzz testing for paddle position updates
========================================================================= */

#include <stdint.h>
#include <string.h>
#include "../paddle.h"

/* Fuzz target: test paddle movement with random inputs
 * Tests boundary clamping and velocity handling
 */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 32) return 0;  // Need minimum data

    /* Parse fuzzer input into paddle state */
    Paddle paddle;
    memcpy(&paddle.position.x, data + 0, sizeof(float));
    memcpy(&paddle.position.y, data + 4, sizeof(float));
    memcpy(&paddle.width, data + 8, sizeof(float));
    memcpy(&paddle.height, data + 12, sizeof(float));
    memcpy(&paddle.velocity, data + 16, sizeof(float));

    int screenHeight = 600;
    if (size >= 24) {
        memcpy(&screenHeight, data + 20, sizeof(int));
        if (screenHeight < 100) screenHeight = 100;
        if (screenHeight > 2000) screenHeight = 2000;
    }

    /* Clamp dimensions to reasonable ranges */
    if (paddle.width < 0.1f) paddle.width = 0.1f;
    if (paddle.width > 100.0f) paddle.width = 100.0f;
    if (paddle.height < 0.1f) paddle.height = 0.1f;
    if (paddle.height > 500.0f) paddle.height = 500.0f;

    /* Test update with various inputs
     * Should handle boundary clamping correctly
     */
    UpdatePaddlePosition(&paddle, screenHeight);

    /* Verify paddle stayed within bounds
     * Note: If paddle.height > screenHeight, paddle will be clamped to y=0
     * and will extend beyond screen bottom, which is acceptable behavior
     */
    if (paddle.position.y < 0.0f) {
        /* Paddle should never have negative position */
        __builtin_trap();
    }
    
    /* For normal-sized paddles, verify they don't exceed bottom */
    if (paddle.height <= (float)screenHeight && 
        paddle.position.y + paddle.height > (float)screenHeight) {
        /* Normal paddle exceeded bottom boundary */
        __builtin_trap();
    }
    
    /* For oversized paddles, verify they're clamped to top */
    if (paddle.height > (float)screenHeight && paddle.position.y > 0.0f) {
        /* Oversized paddle should be at y=0 */
        __builtin_trap();
    }

    /* Test paddle movements */
    MovePaddleUp(&paddle);
    UpdatePaddlePosition(&paddle, screenHeight);

    MovePaddleDown(&paddle);
    UpdatePaddlePosition(&paddle, screenHeight);

    StopPaddle(&paddle);
    UpdatePaddlePosition(&paddle, screenHeight);

    return 0;
}
