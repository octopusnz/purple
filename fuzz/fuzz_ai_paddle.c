/* =========================================================================
    Purple - Fuzz Testing
    https://github.com/octopusnz/purple
    Copyright (c) 2026 Jacob Doherty
    SPDX-License-Identifier: MIT
    See LICENSE.txt for 3rd party library and other resource licenses.
    File: fuzz_ai_paddle.c
    Description: Coverage-guided fuzz testing for AI paddle decision making
========================================================================= */

#include <stdint.h>
#include <string.h>
#include "../paddle.h"

/* Fuzz target: test AI paddle logic with random ball positions and paddle states
 * Tests decision making and boundary handling
 */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 24) return 0;  // Need minimum data

    /* Parse AI paddle state */
    Paddle ai;
    memcpy(&ai.position.x, data + 0, sizeof(float));
    memcpy(&ai.position.y, data + 4, sizeof(float));
    memcpy(&ai.width, data + 8, sizeof(float));
    memcpy(&ai.height, data + 12, sizeof(float));
    ai.velocity = 0.0f;
    ai.score = 0;

    /* Parse ball position */
    Vector2 ballPos;
    memcpy(&ballPos.x, data + 16, sizeof(float));
    memcpy(&ballPos.y, data + 20, sizeof(float));

    float ballRadius = 8.0f;
    int screenHeight = 600;
    
    if (size >= 28) {
        memcpy(&ballRadius, data + 24, sizeof(float));
        if (ballRadius < 1.0f) ballRadius = 1.0f;
        if (ballRadius > 50.0f) ballRadius = 50.0f;
    }
    
    if (size >= 32) {
        memcpy(&screenHeight, data + 28, sizeof(int));
        if (screenHeight < 100) screenHeight = 100;
        if (screenHeight > 2000) screenHeight = 2000;
    }

    /* Clamp paddle dimensions to reasonable ranges */
    if (ai.width < 0.1f) ai.width = 0.1f;
    if (ai.width > 100.0f) ai.width = 100.0f;
    if (ai.height < 0.1f) ai.height = 0.1f;
    if (ai.height > 500.0f) ai.height = 500.0f;

    /* Test AI paddle update multiple times to catch state-dependent bugs */
    for (int i = 0; i < 5; ++i) {
        float prevY = ai.position.y;
        
        UpdateAIPaddle(&ai, ballPos, ballRadius, screenHeight);
        
        /* Verify paddle stayed within bounds
         * Note: Oversized paddles (height > screenHeight) will be at y=0
         */
        if (ai.position.y < 0.0f) {
            /* Paddle should never have negative position */
            __builtin_trap();
        }
        
        /* For normal-sized paddles, verify they don't exceed bottom */
        if (ai.height <= (float)screenHeight && 
            ai.position.y + ai.height > (float)screenHeight) {
            /* Normal paddle exceeded bottom boundary */
            __builtin_trap();
        }
        
        /* For oversized paddles, verify they're clamped to top */
        if (ai.height > (float)screenHeight && ai.position.y > 0.0f) {
            /* Oversized paddle should be at y=0 */
            __builtin_trap();
        }
        
        /* Verify paddle didn't teleport
         * Note: Oversized paddles can jump from any position to y=0 when clamped,
         * so we only check for teleportation on normal-sized paddles.
         * Allow large movements on first iteration to handle out-of-bounds initial positions.
         */
        if (ai.height <= (float)screenHeight && i > 0) {
            float delta = ai.position.y - prevY;
            float maxDelta = (float)screenHeight;
            if (delta < -maxDelta || delta > maxDelta) {
                /* Paddle moved too far in one update */
                __builtin_trap();
            }
        }
        
        /* Modify ball position slightly to test tracking */
        ballPos.y += 10.0f;
        if (ballPos.y > (float)screenHeight) ballPos.y = 0.0f;
    }
    
    return 0;
}
