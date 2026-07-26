/* =========================================================================
    Purple - Fuzz Testing
    https://github.com/octopusnz/purple
    Copyright (c) 2026 Jacob Doherty
    SPDX-License-Identifier: MIT
    See LICENSE.txt for 3rd party library and other resource licenses.
    File: fuzz_ball_update.c
    Description: Fuzz testing for UpdateBallPosition with arbitrary velocity.

    fuzz_ball_wall_sequence.c guards velocity against NaN/Inf at the start,
    so UpdateBallPosition is never actually called with NaN velocity in that
    harness.  This harness leaves velocity raw so libFuzzer can feed NaN/Inf
    and verify the function does not silently corrupt position.

    Without the isfinite guard in ball.c, NaN velocity produces NaN position.
    NaN position then causes IsCollidingVertical to return 0 unconditionally
    (all NaN comparisons are false), silently losing the ball.  This is the
    same class of bug fixed in UpdatePaddlePosition.

    Invariants asserted:
      1. After UpdateBallPosition, position.x is finite.
      2. After UpdateBallPosition, position.y is finite.
      3. IsCollidingVertical returns exactly what its mathematical definition
         computes (correctness oracle), verified only when position is finite
         to avoid a trivially-false oracle for NaN input.

    Input layout (minimum 24 bytes):
      [0..3]   float  ball.position.x  (guarded finite — position is always
                                        initialized to finite in the game)
      [4..7]   float  ball.position.y  (guarded finite)
      [8..11]  float  ball.velocity.x  (raw — NaN/Inf exercised here)
      [12..15] float  ball.velocity.y  (raw — NaN/Inf exercised here)
      [16..19] float  ball.radius      (clamped [1, 50])
      [20..23] int    screenHeight     (clamped [100, 2000])
========================================================================= */

#include <stdint.h>
#include <string.h>
#include <math.h>
#include "../ball.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 24) return 0;

    Ball ball;
    memcpy(&ball.position.x, data +  0, sizeof(float));
    memcpy(&ball.position.y, data +  4, sizeof(float));
    memcpy(&ball.velocity.x, data +  8, sizeof(float));
    memcpy(&ball.velocity.y, data + 12, sizeof(float));
    memcpy(&ball.radius,     data + 16, sizeof(float));

    int screenHeight;
    memcpy(&screenHeight, data + 20, sizeof(int));
    if (screenHeight < 100)  screenHeight = 100;
    if (screenHeight > 2000) screenHeight = 2000;

    /* Clamp radius to a sane range */
    if (!(ball.radius >= 1.0f && ball.radius <= 50.0f)) ball.radius = 8.0f;

    /* Guard initial position: in the game position is always initialized to
     * a known finite value before the physics loop starts.  NaN position is
     * not a valid game state and would mask unrelated NaN-velocity bugs below.
     */
    if (!isfinite(ball.position.x)) ball.position.x = (float)screenHeight;
    if (!isfinite(ball.position.y)) ball.position.y = (float)screenHeight / 2.0f;

    /* velocity is intentionally left raw (including NaN/Inf) so libFuzzer can
     * explore the NaN-propagation path that the existing wall-sequence harness
     * explicitly avoids.
     */

    UpdateBallPosition(&ball);

    /* --- Invariant 1 & 2: position must be finite after the update ---
     * Without an isfinite guard in UpdateBallPosition, NaN velocity produces
     * NaN position which silently bypasses IsCollidingVertical checks below.
     */
    if (!isfinite(ball.position.x)) {
        /* position.x became NaN or Inf — NaN velocity propagated */
        __builtin_trap();
    }
    if (!isfinite(ball.position.y)) {
        /* position.y became NaN or Inf — NaN velocity propagated */
        __builtin_trap();
    }

    /* --- Invariant 3: IsCollidingVertical correctness oracle ---
     * Only run when position is finite (guaranteed by invariants 1 & 2 above).
     */
    int got  = IsCollidingVertical(&ball, screenHeight);
    int want = (ball.position.y + ball.radius >= (float)screenHeight) ||
               (ball.position.y - ball.radius <= 0.0f);
    if (got != want) {
        /* IsCollidingVertical returned wrong answer */
        __builtin_trap();
    }

    return 0;
}
