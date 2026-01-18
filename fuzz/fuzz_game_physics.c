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
#include "../ball.h"
#include "../paddle.h"

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

    int screenHeight = 600;

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

    return 0;
}
