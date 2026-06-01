/* =========================================================================
    Purple - Fuzz Testing
    https://github.com/octopusnz/purple
    Copyright (c) 2026 Jacob Doherty
    SPDX-License-Identifier: MIT
    See LICENSE.txt for 3rd party library and other resource licenses.
    File: fuzz_ball_wall_sequence.c
    Description: Fuzz testing for multi-frame ball wall-bounce sequences.

    Tests the UpdateBallPosition + IsCollidingVertical interaction over many
    frames and asserts velocity-magnitude preservation invariants that no
    other harness checks:

      1. vx is NEVER modified by a wall bounce — only velocity.y is flipped.
         After any number of wall bounces, ball.velocity.x must equal its
         initial value exactly (IEEE 754 sign flip preserves the bit pattern).

      2. |vy| is preserved by wall bounces.  velocity.y *= -1.0f is a pure
         sign flip; it cannot change magnitude for any finite float.  After
         every bounce, fabsf(ball.velocity.y) must equal the initial |vy|.

      3. Neither velocity component becomes NaN or Inf during the sequence.

      4. IsCollidingVertical must agree with its own mathematical definition:
         the return value must match the expression
           (y + r >= screenHeight) || (y - r <= 0).
         This is the same correctness oracle used in fuzz_ball_collision.c but
         here it is checked on every frame of a multi-bounce sequence.

    The harness simulates ONLY wall bounces (no paddle collisions), isolating
    the UpdateBallPosition / IsCollidingVertical / velocity-flip code path.

    Input layout (minimum 21 bytes):
      [0..3]   float   ball.position.x   (any finite value, clamped to screen)
      [4..7]   float   ball.position.y   (any finite value, clamped to screen)
      [8..11]  float   ball.velocity.x   (any finite value; NaN → 0)
      [12..15] float   ball.velocity.y   (any finite value; 0 is interesting)
      [16..19] int     screenHeight       (clamped to [100, 2000])
      [20]     uint8   ball.radius hint   (mapped to [1.0f, 30.0f])
      [21]     uint8   frame count hint   (mapped to [1, 200])
========================================================================= */

#include <stdint.h>
#include <string.h>
#include <math.h>
#include "../ball.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 21) return 0;

    Ball ball;
    memcpy(&ball.position.x, data +  0, sizeof(float));
    memcpy(&ball.position.y, data +  4, sizeof(float));
    memcpy(&ball.velocity.x, data +  8, sizeof(float));
    memcpy(&ball.velocity.y, data + 12, sizeof(float));

    int screenHeight;
    memcpy(&screenHeight, data + 16, sizeof(int));
    if (screenHeight < 100)  screenHeight = 100;
    if (screenHeight > 2000) screenHeight = 2000;

    /* Map radius byte to [1.0, 30.0] */
    ball.radius = 1.0f + (float)(data[20] % 30);

    /* Map frame count byte to [1, 200] if present, else default 100 */
    int nframes = 100;
    if (size >= 22) {
        nframes = 1 + (int)(data[21] % 200);
    }

    /* Guard against NaN/Inf initial state; these are not valid game states */
    if (!isfinite(ball.position.x)) ball.position.x = (float)screenHeight / 2.0f;
    if (!isfinite(ball.position.y)) ball.position.y = (float)screenHeight / 2.0f;
    if (!isfinite(ball.velocity.x)) ball.velocity.x = 0.0f;
    if (!isfinite(ball.velocity.y)) ball.velocity.y = 1.0f;

    /* Clamp velocities to a sane magnitude to avoid position overflow
     * over many frames; mirrors typical game speeds (well below these limits).
     */
    if (ball.velocity.x >  500.0f) ball.velocity.x =  500.0f;
    if (ball.velocity.x < -500.0f) ball.velocity.x = -500.0f;
    if (ball.velocity.y >  500.0f) ball.velocity.y =  500.0f;
    if (ball.velocity.y < -500.0f) ball.velocity.y = -500.0f;

    /* Record initial velocity components for the invariant checks */
    const float init_vx       = ball.velocity.x;
    const float init_vy_abs   = fabsf(ball.velocity.y);

    for (int frame = 0; frame < nframes; ++frame) {

        /* ----- Invariant 4: IsCollidingVertical must match its definition ---- */
        {
            int got  = IsCollidingVertical(&ball, screenHeight);
            int want = (ball.position.y + ball.radius >= (float)screenHeight) ||
                       (ball.position.y - ball.radius <= 0.0f);
            if (got != want) {
                __builtin_trap();
            }

            /* Apply the wall bounce (mirrors the game's main loop) */
            if (got) {
                ball.velocity.y *= -1.0f;
            }
        }

        /* ----- Invariant 1: vx unchanged by wall bounces ----- */
        if (ball.velocity.x != init_vx) {
            __builtin_trap();
        }

        /* ----- Invariant 2: |vy| preserved by wall bounces ----- */
        if (fabsf(ball.velocity.y) != init_vy_abs) {
            __builtin_trap();
        }

        /* ----- Invariant 3: velocities remain finite ----- */
        if (!isfinite(ball.velocity.x) || !isfinite(ball.velocity.y)) {
            __builtin_trap();
        }

        /* Advance ball position */
        UpdateBallPosition(&ball);

        /* Positions must also remain finite after update */
        if (!isfinite(ball.position.x) || !isfinite(ball.position.y)) {
            __builtin_trap();
        }
    }

    /* ----- Edge case: NULL ball must not crash any function ----- */
    IsCollidingVertical(NULL, screenHeight);
    UpdateBallPosition(NULL);

    return 0;
}
