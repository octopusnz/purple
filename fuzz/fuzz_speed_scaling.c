/* =========================================================================
    Purple - Fuzz Testing
    https://github.com/octopusnz/purple
    Copyright (c) 2026 Jacob Doherty
    SPDX-License-Identifier: MIT
    See LICENSE.txt for 3rd party library and other resource licenses.
    File: fuzz_speed_scaling.c
    Description: Coverage-guided fuzz testing for speed multiplier and ball
                 velocity initialization (mirrors the static CalculateSpeedMultiplier
                 and ResetBall logic in main.c without requiring Raylib).
========================================================================= */

#include <stdint.h>
#include <string.h>
#include <math.h>
#include "../ball.h"

/* Mirror of the constants and functions defined as static in main.c.
 * Kept here so the fuzz harness can test the arithmetic without pulling in
 * Raylib's rendering context.
 */
#define BALL_INITIAL_SPEED_X    6.0f
#define BALL_INITIAL_SPEED_Y    3.0f
#define SPEED_INCREMENT_PER_POINT 0.12f
#define SCREEN_WIDTH  1200
#define SCREEN_HEIGHT  600

static float FuzzCalculateSpeedMultiplier(int totalScore)
{
    return 1.0f + (float)totalScore * SPEED_INCREMENT_PER_POINT;
}

/* Simulate ResetBall velocity calculation (rand() direction omitted; we test
 * both directions explicitly).
 */
static void FuzzResetBall(Ball *ball, float speedMultiplier, int dirX, int dirY)
{
    ball->position.x = (float)SCREEN_WIDTH  / 2.0f;
    ball->position.y = (float)SCREEN_HEIGHT / 2.0f;
    ball->velocity.x = BALL_INITIAL_SPEED_X * speedMultiplier * (float)(dirX >= 0 ? 1 : -1);
    ball->velocity.y = BALL_INITIAL_SPEED_Y * speedMultiplier * (float)(dirY >= 0 ? 1 : -1);
}

/* Fuzz target: exercise speed multiplier arithmetic and resulting ball velocities
 *
 * Input layout (minimum 9 bytes):
 *   [0..3]  int32  totalScore  — treated as signed; arbitrary range explored
 *   [4]     uint8  dirX        — high bit → negative X direction
 *   [5]     uint8  dirY        — high bit → negative Y direction
 *   [6..9]  float  extraSpeed  — additional speed multiplier overlay (optional)
 */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 6) return 0;

    int totalScore;
    memcpy(&totalScore, data, sizeof(int));

    int dirX = (int)(data[4] & 0x80) ? -1 : 1;
    int dirY = (int)(data[5] & 0x80) ? -1 : 1;

    /* --- 1. Multiplier sanity ---
     * For any non-negative score the multiplier must be >= 1.0 and finite.
     * Negative totalScore (data-driven) is allowed by the formula; the result
     * can be <1 or even <=0 — but must never be NaN.
     */
    float multiplier = FuzzCalculateSpeedMultiplier(totalScore);

    /* NaN check using the self-inequality property */
    if (multiplier != multiplier) {
        /* NaN in speed multiplier — must never happen for integer input */
        __builtin_trap();
    }

    /* For the range the game actually uses (0 to 9 points, i.e. 0–9 total)
     * the multiplier must be in [1.0, ~2.08] and finite.
     */
    if (totalScore >= 0 && totalScore <= 9) {
        if (!(multiplier >= 1.0f && multiplier <= 2.1f)) {
            __builtin_trap();
        }
    }

    /* --- 2. Velocity finiteness ---
     * After applying the multiplier the ball velocities must be finite
     * (not Inf, -Inf, or NaN) for reasonable scores. Scores in the range
     * INT_MIN..INT_MAX can overflow float and produce Inf; we only assert
     * on game-realistic scores where Inf must not appear.
     */
    Ball ball;
    ball.radius = 8.0f;
    FuzzResetBall(&ball, multiplier, dirX, dirY);

    /* Game-realistic scores: assert strict finiteness */
    if (totalScore >= 0 && totalScore <= 9) {
        if (!isfinite(ball.velocity.x)) __builtin_trap();
        if (!isfinite(ball.velocity.y)) __builtin_trap();
        if (!isfinite(ball.position.x)) __builtin_trap();
        if (!isfinite(ball.position.y)) __builtin_trap();
    }

    /* --- 3. Propagation through UpdateBallPosition ---
     * After one update step, finite velocities must produce finite positions.
     */
    if (isfinite(ball.velocity.x) && isfinite(ball.velocity.y) &&
        isfinite(ball.position.x) && isfinite(ball.position.y))
    {
        UpdateBallPosition(&ball);
        if (!isfinite(ball.position.x)) __builtin_trap();
        if (!isfinite(ball.position.y)) __builtin_trap();
    }

    /* --- 4. Extreme score simulation ---
     * Use fuzz-derived totalScore without restricting range; check that the
     * formula cannot produce NaN (Inf is allowed for extreme inputs, but NaN
     * indicates a logic error such as 0 * Inf or Inf - Inf).
     */
    float extremeMultiplier = FuzzCalculateSpeedMultiplier(totalScore);
    if (extremeMultiplier != extremeMultiplier) {
        /* NaN: formula is broken for this input */
        __builtin_trap();
    }

    /* --- 5. Optional extra-speed overlay ---
     * Some callers might compose multipliers; verify that chained
     * multiplication of two finite multipliers stays finite when inputs are
     * within [1, 10] (representative of power-ups / future features).
     */
    if (size >= 10) {
        float extraSpeed;
        memcpy(&extraSpeed, data + 6, sizeof(float));
        if (isfinite(extraSpeed) && extraSpeed >= 1.0f && extraSpeed <= 10.0f &&
            isfinite(multiplier))
        {
            float composed = multiplier * extraSpeed;
            Ball ball2;
            ball2.radius = 8.0f;
            FuzzResetBall(&ball2, composed, dirX, dirY);
            /* Composed result with bounded inputs must also be finite */
            if (!isfinite(ball2.velocity.x)) __builtin_trap();
            if (!isfinite(ball2.velocity.y)) __builtin_trap();
        }
    }

    return 0;
}
