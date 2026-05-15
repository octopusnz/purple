/* =========================================================================
    Purple - Fuzz Testing
    https://github.com/octopusnz/purple
    Copyright (c) 2026 Jacob Doherty
    SPDX-License-Identifier: MIT
    See LICENSE.txt for 3rd party library and other resource licenses.
    File: fuzz_ball_collision.c
    Description: Coverage-guided fuzz testing for ball collision detection
========================================================================= */

#include <stdint.h>
#include <string.h>
#include "../ball.h"

/* Fuzz target: test ball collision with random paddle positions and states
 * This harness feeds random binary data to collision detection functions,
 * allowing libFuzzer to discover edge cases and undefined behavior.
 */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 32) return 0;  // Need minimum 32 bytes for our test inputs

    /* Parse fuzzer input into ball state
     * Layout: 4 floats (16 bytes) + 2 floats (8 bytes) + 1 float (4 bytes) + 3 bytes padding
     */
    Ball ball;
    memcpy(&ball.position.x, data + 0, sizeof(float));
    memcpy(&ball.position.y, data + 4, sizeof(float));
    memcpy(&ball.velocity.x, data + 8, sizeof(float));
    memcpy(&ball.velocity.y, data + 12, sizeof(float));
    memcpy(&ball.radius, data + 16, sizeof(float));

    /* Parse paddle position from remaining data */
    Vector2 paddlePos;
    memcpy(&paddlePos.x, data + 20, sizeof(float));
    memcpy(&paddlePos.y, data + 24, sizeof(float));

    /* Use remaining bytes to vary paddle dimensions if available */
    float paddleWidth = 15.0f;
    float paddleHeight = 100.0f;
    if (size >= 36) {
        memcpy(&paddleWidth, data + 28, sizeof(float));
    }
    if (size >= 40) {
        memcpy(&paddleHeight, data + 32, sizeof(float));
    }

    /* Clamp paddle dimensions to reasonable ranges to avoid extreme values */
    if (paddleWidth < 0.1f) paddleWidth = 0.1f;
    if (paddleWidth > 100.0f) paddleWidth = 100.0f;
    if (paddleHeight < 0.1f) paddleHeight = 0.1f;
    if (paddleHeight > 500.0f) paddleHeight = 500.0f;
    if (ball.radius < 0.1f) ball.radius = 0.1f;
    if (ball.radius > 50.0f) ball.radius = 50.0f;

    /* Test collision handling with random inputs
     * These functions should never crash regardless of input
     */
    HandlePaddleCollision(&ball, paddlePos, paddleWidth, paddleHeight);

    /* Test vertical collision with various screen heights */
    int screenHeight = 600;
    if (size >= 44) {
        memcpy(&screenHeight, data + 40, sizeof(int));
        if (screenHeight < 100) screenHeight = 100;
        if (screenHeight > 2000) screenHeight = 2000;
    }
    IsCollidingVertical(&ball, screenHeight);

    /* Update position (should handle any velocity) */
    UpdateBallPosition(&ball);

    /* Test collision again after update to ensure consistency */
    HandlePaddleCollision(&ball, paddlePos, paddleWidth, paddleHeight);

    /* Spin accumulation invariant: repeated collisions must not push velocity.y
     * beyond the MAX_BALL_SPEED_Y cap (15.0f) defined in ball.c.
     *
     * A fresh ball is used with NaN-safe, clamped paddle geometry so the test
     * is independent of any NaN state that may have arrived via the fuzz input.
     * The ball is repositioned inside the paddle before each call to guarantee
     * a collision actually fires and the spin logic runs every iteration.
     */
    {
        Vector2 spinPos;
        float   spinW, spinH;

        /* Use fuzz-derived paddle values; fall back to defaults for NaN inputs */
        spinPos.x = (paddlePos.x == paddlePos.x) ? paddlePos.x : 20.0f;
        spinPos.y = (paddlePos.y == paddlePos.y) ? paddlePos.y : 200.0f;
        spinW     = (paddleWidth  == paddleWidth)  ? paddleWidth  : 15.0f;
        spinH     = (paddleHeight == paddleHeight) ? paddleHeight : 100.0f;

        Ball spinBall;
        spinBall.velocity.x = 4.0f;
        spinBall.velocity.y = 0.0f;
        spinBall.radius     = ball.radius;  /* already clamped above */

        for (int bounce = 0; bounce < 20; ++bounce) {
            /* Place ball at paddle centre to guarantee collision */
            spinBall.position.x = spinPos.x + spinW / 2.0f;
            spinBall.position.y = spinPos.y + spinH / 2.0f;

            HandlePaddleCollision(&spinBall, spinPos, spinW, spinH);

            float vy = spinBall.velocity.y;
            /* Use negative-form so NaN (which fails all comparisons) also traps */
            if (!(vy >= -15.0f && vy <= 15.0f)) {
                __builtin_trap();
            }
        }
    }

    return 0;
}
