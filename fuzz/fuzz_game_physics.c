/* =========================================================================
    Purple - Fuzz Testing
    https://github.com/octopusnz/purple
    Copyright (c) 2026 Jacob Doherty
    SPDX-License-Identifier: MIT
    See LICENSE.txt for 3rd party library and other resource licenses.
    File: fuzz_game_physics.c
    Description: Coverage-guided fuzz testing for combined ball and paddle physics
========================================================================= */

#include <stdint.h>
#include <string.h>
#include <math.h>
#include "../ball.h"
#include "../paddle.h"

/* Mirror of main.c constants (cannot include main.c — Raylib dependency) */
#define FUZZ_BALL_INITIAL_SPEED_X   6.0f
#define FUZZ_BALL_INITIAL_SPEED_Y   3.0f
#define FUZZ_SPEED_INCREMENT        0.12f

/* Fuzz target: test realistic game scenarios with ball and paddle interactions
 * Tests combined physics simulation over multiple frames
 */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 60) return 0;  // Need data for ball + 2 paddles

    /* Parse ball state */
    Ball ball;
    memcpy(&ball.position.x, data + 0, sizeof(float));
    memcpy(&ball.position.y, data + 4, sizeof(float));
    memcpy(&ball.velocity.x, data + 8, sizeof(float));
    memcpy(&ball.velocity.y, data + 12, sizeof(float));
    memcpy(&ball.radius, data + 16, sizeof(float));

    /* Parse player paddle */
    Paddle player;
    memcpy(&player.position.x, data + 20, sizeof(float));
    memcpy(&player.position.y, data + 24, sizeof(float));
    memcpy(&player.width, data + 28, sizeof(float));
    memcpy(&player.height, data + 32, sizeof(float));
    memcpy(&player.velocity, data + 36, sizeof(float));
    player.score = 0;

    /* Parse AI paddle */
    Paddle ai;
    memcpy(&ai.position.x, data + 40, sizeof(float));
    memcpy(&ai.position.y, data + 44, sizeof(float));
    memcpy(&ai.width, data + 48, sizeof(float));
    memcpy(&ai.height, data + 52, sizeof(float));
    ai.velocity = 0.0f;
    ai.score = 0;

    /* Parse screenHeight from bytes 56–59 (within the existing 60-byte minimum).
     * Clamped to a sane range so paddle/ball edge cases at different heights are
     * explored without triggering degenerate geometry.
     */
    int screenHeight = 600;
    memcpy(&screenHeight, data + 56, sizeof(int));
    if (screenHeight < 100) screenHeight = 100;
    if (screenHeight > 2000) screenHeight = 2000;

    /* Clamp values to reasonable ranges to avoid NaN/Inf propagation */
    /* Ball properties */
    if (ball.radius < 1.0f || ball.radius != ball.radius) ball.radius = 8.0f;
    if (ball.radius > 50.0f) ball.radius = 50.0f;
    
    /* Clamp ball position and velocity */
    if (ball.position.x != ball.position.x) ball.position.x = 600.0f;
    if (ball.position.y != ball.position.y) ball.position.y = 300.0f;
    if (ball.velocity.x != ball.velocity.x) ball.velocity.x = 4.0f;
    if (ball.velocity.y != ball.velocity.y) ball.velocity.y = 2.0f;
    
    /* Clamp to reasonable bounds */
    if (ball.position.x < -100.0f) ball.position.x = -100.0f;
    if (ball.position.x > 1300.0f) ball.position.x = 1300.0f;
    if (ball.position.y < -100.0f) ball.position.y = -100.0f;
    if (ball.position.y > 700.0f) ball.position.y = 700.0f;
    if (ball.velocity.x < -50.0f) ball.velocity.x = -50.0f;
    if (ball.velocity.x > 50.0f) ball.velocity.x = 50.0f;
    if (ball.velocity.y < -50.0f) ball.velocity.y = -50.0f;
    if (ball.velocity.y > 50.0f) ball.velocity.y = 50.0f;
    
    /* Player paddle */
    if (player.width < 0.1f || player.width != player.width) player.width = 15.0f;
    if (player.width > 100.0f) player.width = 100.0f;
    if (player.height < 0.1f || player.height != player.height) player.height = 100.0f;
    if (player.height > 500.0f) player.height = 500.0f;
    if (player.position.x != player.position.x) player.position.x = 20.0f;
    if (player.position.y != player.position.y) player.position.y = 250.0f;
    if (player.velocity != player.velocity) player.velocity = 0.0f;
    if (player.velocity < -50.0f) player.velocity = -50.0f;
    if (player.velocity > 50.0f) player.velocity = 50.0f;
    
    /* AI paddle */
    if (ai.width < 0.1f || ai.width != ai.width) ai.width = 15.0f;
    if (ai.width > 100.0f) ai.width = 100.0f;
    if (ai.height < 0.1f || ai.height != ai.height) ai.height = 100.0f;
    if (ai.height > 500.0f) ai.height = 500.0f;
    if (ai.position.x != ai.position.x) ai.position.x = 1165.0f;
    if (ai.position.y != ai.position.y) ai.position.y = 250.0f;

    /* Simulate 100 frames of gameplay */
    for (int frame = 0; frame < 100; ++frame) {
        /* Update ball position */
        UpdateBallPosition(&ball);

        /* Check vertical wall collisions */
        if (IsCollidingVertical(&ball, screenHeight)) {
            ball.velocity.y *= -1.0f;
        }

        /* Update paddles */
        UpdatePaddlePosition(&player, screenHeight);
        UpdateAIPaddle(&ai, ball.position, ball.radius, screenHeight);

        /* Check paddle collisions */
        HandlePaddleCollision(&ball, player.position, player.width, player.height);
        HandlePaddleCollision(&ball, ai.position, ai.width, ai.height);

        /* Clamp velocities to prevent unbounded growth from spin effects */
        if (ball.velocity.x > 100.0f) ball.velocity.x = 100.0f;
        if (ball.velocity.x < -100.0f) ball.velocity.x = -100.0f;
        if (ball.velocity.y > 100.0f) ball.velocity.y = 100.0f;
        if (ball.velocity.y < -100.0f) ball.velocity.y = -100.0f;

        /* Verify physics invariants */
        if (ball.position.y < -1000.0f || ball.position.y > (float)screenHeight + 1000.0f) {
            /* Ball escaped vertically beyond tolerance */
            __builtin_trap();
        }

        /* Player paddle boundary checks
         * Oversized paddles (height > screenHeight) are clamped to y=0
         */
        if (player.position.y < 0.0f) {
            /* Paddle should never have negative position */
            __builtin_trap();
        }
        if (player.height <= (float)screenHeight && 
            player.position.y + player.height > (float)screenHeight) {
            /* Normal paddle exceeded bottom boundary */
            __builtin_trap();
        }
        if (player.height > (float)screenHeight && player.position.y > 0.1f) {
            /* Oversized paddle should be at y=0 (allow small tolerance) */
            __builtin_trap();
        }

        /* AI paddle boundary checks */
        if (ai.position.y < 0.0f) {
            /* Paddle should never have negative position */
            __builtin_trap();
        }
        if (ai.height <= (float)screenHeight && 
            ai.position.y + ai.height > (float)screenHeight) {
            /* Normal paddle exceeded bottom boundary */
            __builtin_trap();
        }
        if (ai.height > (float)screenHeight && ai.position.y > 0.1f) {
            /* Oversized paddle should be at y=0 (allow small tolerance) */
            __builtin_trap();
        }

        /* Check for NaN or infinite values */
        if (ball.position.x != ball.position.x ||  /* NaN check */
            ball.position.y != ball.position.y ||
            ball.velocity.x != ball.velocity.x ||
            ball.velocity.y != ball.velocity.y) {
            /* Invalid floating point values detected */
            __builtin_trap();
        }
    }

    /* --- Score-based speed scaling simulation ---
     * Mirrors the main-loop path: score point → CalculateSpeedMultiplier →
     * ResetBall with new velocity → continue physics.  This path was
     * previously untested because the loop above uses a fixed speed.
     *
     * byte[60]: encodes totalScore in [0, 9] (game-realistic range).
     * bytes[61..64]: optional float for a second score tier.
     */
    if (size >= 61) {
        int totalScore = (int)(data[60] % 10);
        float speedMul = 1.0f + (float)totalScore * FUZZ_SPEED_INCREMENT;

        /* Multiplier must be finite and in expected range for 0-9 points */
        if (!isfinite(speedMul))           __builtin_trap();
        if (!(speedMul >= 1.0f && speedMul <= 2.1f)) __builtin_trap();

        /* Fresh ball at screen centre with speed-scaled velocity */
        Ball sb;
        sb.radius     = 8.0f;
        sb.position.x = 600.0f;
        sb.position.y = (float)screenHeight / 2.0f;
        sb.velocity.x = FUZZ_BALL_INITIAL_SPEED_X * speedMul *
                        (ball.velocity.x >= 0.0f ? 1.0f : -1.0f);
        sb.velocity.y = FUZZ_BALL_INITIAL_SPEED_Y * speedMul *
                        (ball.velocity.y >= 0.0f ? 1.0f : -1.0f);

        if (!isfinite(sb.velocity.x)) __builtin_trap();
        if (!isfinite(sb.velocity.y)) __builtin_trap();

        /* Run 30 frames; HandlePaddleCollision's spin cap keeps vy bounded,
         * vx only flips sign → both must stay finite with no external clamp.
         */
        for (int f = 0; f < 30; ++f) {
            UpdateBallPosition(&sb);

            if (IsCollidingVertical(&sb, screenHeight)) {
                sb.velocity.y *= -1.0f;
            }

            HandlePaddleCollision(&sb, player.position, player.width,  player.height);
            HandlePaddleCollision(&sb, ai.position,     ai.width,      ai.height);

            if (!isfinite(sb.velocity.x)) __builtin_trap();
            if (!isfinite(sb.velocity.y)) __builtin_trap();
            if (!isfinite(sb.position.x)) __builtin_trap();
            if (!isfinite(sb.position.y)) __builtin_trap();
        }
    }

    return 0;
}
