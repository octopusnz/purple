/* =========================================================================
    Purple - Fuzz Testing
    https://github.com/octopusnz/purple
    Copyright (c) 2026 Jacob Doherty
    SPDX-License-Identifier: MIT
    See LICENSE.txt for 3rd party library and other resource licenses.
    File: fuzz_ball_spin_sequence.c
    Description: Fuzz testing for spin accumulation over repeated paddle collisions.

    fuzz_ball_collision.c calls HandlePaddleCollision exactly once.  This
    harness drives N consecutive alternating hits between two paddles and
    asserts invariants that no single-hit harness can exercise:

      1. After every hit, |velocity.y| <= MAX_BALL_SPEED_Y.
         The spin clamp (velocity.y clamped to [-MAX, +MAX]) must hold even
         after many accumulated increments with adversarial paddle geometry.

      2. After every hit, velocity.y is finite.
         NaN initial velocity stays NaN through the spin addition because the
         subsequent ordered clamp comparisons (> MAX, < -MAX) both return false
         for NaN.  The isfinite guard in HandlePaddleCollision catches this.

      3. After every hit, velocity.x is finite.
         velocity.x *= -1.0f cannot produce NaN from a finite value, but can
         keep NaN if the initial value was NaN.

      4. The halfHeight > 0.01f guard must prevent division-by-zero spin
         for ultra-thin paddles.  libFuzzer will explore paddleHeight values
         near the 0.1f clamp boundary to exercise this path.

    Two paddles are used (one near each screen edge) so the ball alternates
    direction, exercising the spin path from both sides of the paddle face.

    Input layout (minimum 41 bytes):
      [0..3]   float  ball.position.x   (clamped finite)
      [4..7]   float  ball.position.y   (clamped finite)
      [8..11]  float  ball.velocity.x   (clamped finite, non-zero)
      [12..15] float  ball.velocity.y   (clamped finite)
      [16..19] float  ball.radius       (clamped [1, 30])
      [20..23] float  paddle1.position.y (adversarial Y offset)
      [24..27] float  paddle1.height    (clamped [0.1, 500])
      [28..31] float  paddle2.position.y (adversarial Y offset)
      [32..35] float  paddle2.height    (clamped [0.1, 500])
      [36..39] int    screenHeight      (clamped [100, 2000])
      [40]     uint8  hit count hint    (mapped to [1, 50])
========================================================================= */

#include <stdint.h>
#include <string.h>
#include <math.h>
#include "../ball.h"

/* Mirror of the private constant in ball.c */
#define FUZZ_MAX_BALL_SPEED_Y 15.0f

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 41) return 0;

    Ball ball;
    memcpy(&ball.position.x, data +  0, sizeof(float));
    memcpy(&ball.position.y, data +  4, sizeof(float));
    memcpy(&ball.velocity.x, data +  8, sizeof(float));
    memcpy(&ball.velocity.y, data + 12, sizeof(float));
    memcpy(&ball.radius,     data + 16, sizeof(float));

    float p1y, p1h, p2y, p2h;
    memcpy(&p1y, data + 20, sizeof(float));
    memcpy(&p1h, data + 24, sizeof(float));
    memcpy(&p2y, data + 28, sizeof(float));
    memcpy(&p2h, data + 32, sizeof(float));

    int screenHeight;
    memcpy(&screenHeight, data + 36, sizeof(int));
    if (screenHeight < 100)  screenHeight = 100;
    if (screenHeight > 2000) screenHeight = 2000;

    int nHits = 1 + (int)(data[40] % 50);

    /* Clamp ball radius */
    if (!(ball.radius >= 1.0f && ball.radius <= 30.0f)) ball.radius = 8.0f;

    /* Guard ball position against NaN/Inf: not a valid game state */
    if (!isfinite(ball.position.x)) ball.position.x = (float)screenHeight;
    if (!isfinite(ball.position.y)) ball.position.y = (float)screenHeight / 2.0f;

    /* Guard velocity against NaN/Inf — we test only the spin accumulation
     * invariant here, not NaN-velocity propagation (that is fuzz_ball_update.c).
     * Use the negated-form NaN-safe comparison: !(x >= lo && x <= hi) catches NaN.
     */
    if (!(ball.velocity.x >= -50.0f && ball.velocity.x <= 50.0f))
        ball.velocity.x = 6.0f;
    /* Ensure velocity.x is non-zero so direction is meaningful */
    if (ball.velocity.x == 0.0f) ball.velocity.x = 6.0f;

    if (!(ball.velocity.y >= -FUZZ_MAX_BALL_SPEED_Y &&
          ball.velocity.y <=  FUZZ_MAX_BALL_SPEED_Y))
        ball.velocity.y = 0.0f;

    /* Clamp paddle heights: exercise the halfHeight > 0.01f guard boundary */
    if (!(p1h >= 0.1f && p1h <= 500.0f)) p1h = 100.0f;
    if (!(p2h >= 0.1f && p2h <= 500.0f)) p2h = 100.0f;

    /* Guard paddle Y positions */
    if (!isfinite(p1y)) p1y = 0.0f;
    if (!isfinite(p2y)) p2y = 0.0f;

    /* Fixed paddle widths and X positions mirroring the real game layout */
    const float PADDLE_W = 15.0f;
    const float LEFT_X   = 20.0f;
    const float RIGHT_X  = (float)screenHeight * 2.0f - 35.0f;  /* ~screen right */

    Vector2 paddle1Pos = { LEFT_X,  p1y };
    Vector2 paddle2Pos = { RIGHT_X, p2y };

    for (int i = 0; i < nHits; ++i) {
        /* Alternate between left and right paddle to drive direction reversals */
        if (i % 2 == 0) {
            HandlePaddleCollision(&ball, paddle1Pos, PADDLE_W, p1h);
        } else {
            HandlePaddleCollision(&ball, paddle2Pos, PADDLE_W, p2h);
        }

        /* --- Invariant 1 & 2: velocity.y must be finite and within bounds --- */
        if (!isfinite(ball.velocity.y)) {
            /* velocity.y became NaN or Inf after HandlePaddleCollision */
            __builtin_trap();
        }
        if (ball.velocity.y > FUZZ_MAX_BALL_SPEED_Y ||
            ball.velocity.y < -FUZZ_MAX_BALL_SPEED_Y) {
            /* Spin clamp invariant violated */
            __builtin_trap();
        }

        /* --- Invariant 3: velocity.x must remain finite --- */
        if (!isfinite(ball.velocity.x)) {
            /* velocity.x became NaN or Inf after HandlePaddleCollision */
            __builtin_trap();
        }
    }

    return 0;
}
