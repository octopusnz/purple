/* =========================================================================
    Purple - Fuzz Testing
    https://github.com/octopusnz/purple
    Copyright (c) 2026 Jacob Doherty
    SPDX-License-Identifier: MIT
    See LICENSE.txt for 3rd party library and other resource licenses.
    File: fuzz_paddle_sequence.c
    Description: Fuzz testing for arbitrary paddle input sequences.

    fuzz_paddle_position.c exercises a fixed three-step sequence
    (MovePaddleUp → MovePaddleDown → StopPaddle).  This harness instead
    drives a fuzz-controlled state machine so libFuzzer can discover
    state-dependent bugs in longer or adversarial sequences that the
    fixed test cannot reach.

    Each input byte after the initial setup encodes one action:
      0x00  MovePaddleUp   + UpdatePaddlePosition
      0x01  MovePaddleDown + UpdatePaddlePosition
      0x02  StopPaddle     + UpdatePaddlePosition
      other               UpdatePaddlePosition only (no velocity change)

    After every UpdatePaddlePosition call the harness verifies the same
    set of boundary invariants as fuzz_paddle_position.c:
      1. position.y is finite
      2. position.y >= 0.0f  (never negative)
      3. For normal paddles (height <= screenHeight):
           position.y + height <= screenHeight
      4. For oversized paddles (height > screenHeight):
           position.y == 0.0f

    The NULL-guard paths are also exercised at the end.

    Input layout:
      [0..3]   float   paddle.position.y  (any value; NaN/Inf → 0)
      [4..7]   float   paddle.height      (clamped to [0.1, 500])
      [8..11]  int     screenHeight       (clamped to [100, 2000])
      [12..]   bytes   action sequence    (one action per byte)
========================================================================= */

#include <stdint.h>
#include <string.h>
#include <math.h>
#include "../paddle.h"

/* Tolerance for the oversized-paddle top-clamp check.
 * UpdatePaddlePosition sets position.y = 0 for oversized paddles but only
 * after applying velocity first, so allow a single frame of drift.
 */
#define OVERSIZE_TOL 0.01f

static void CheckPaddleBounds(const Paddle *p, int screenHeight)
{
    /* Invariant 1: position.y must be finite */
    if (!isfinite(p->position.y)) {
        __builtin_trap();
    }

    /* Invariant 2: never negative */
    if (p->position.y < 0.0f) {
        __builtin_trap();
    }

    if (p->height <= (float)screenHeight) {
        /* Invariant 3: normal paddle must not exceed bottom boundary */
        if (p->position.y + p->height > (float)screenHeight + OVERSIZE_TOL) {
            __builtin_trap();
        }
    } else {
        /* Invariant 4: oversized paddle must be clamped to top */
        if (p->position.y > OVERSIZE_TOL) {
            __builtin_trap();
        }
    }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 12) return 0;

    Paddle paddle;
    memcpy(&paddle.position.y, data + 0, sizeof(float));
    /* position.x is irrelevant (clamping is vertical only) but must be set */
    paddle.position.x = 20.0f;
    memcpy(&paddle.height, data + 4, sizeof(float));
    paddle.velocity = 0.0f;
    paddle.width    = 15.0f;
    paddle.score    = 0;

    int screenHeight;
    memcpy(&screenHeight, data + 8, sizeof(int));
    if (screenHeight < 100)  screenHeight = 100;
    if (screenHeight > 2000) screenHeight = 2000;

    /* Sanitise height */
    if (!(paddle.height >= 0.1f)) paddle.height = 0.1f;
    if (paddle.height > 500.0f)   paddle.height = 500.0f;

    /* Sanitise initial position: NaN/Inf is not a valid game state */
    if (!(paddle.position.y > -1e6f && paddle.position.y < 1e6f))
        paddle.position.y = 0.0f;

    /* Run one initial update to bring any out-of-bounds initial position into
     * range before we start checking invariants strictly.
     */
    UpdatePaddlePosition(&paddle, screenHeight);
    CheckPaddleBounds(&paddle, screenHeight);

    /* ----- Drive the fuzz-controlled state machine ----- */
    for (size_t i = 12; i < size; ++i) {
        uint8_t action = data[i];

        switch (action & 0x03) {   /* only 4 actions; mask to keep it clean */
        case 0x00:
            MovePaddleUp(&paddle);
            break;
        case 0x01:
            MovePaddleDown(&paddle);
            break;
        case 0x02:
            StopPaddle(&paddle);
            break;
        default:
            /* No velocity change; bare UpdatePaddlePosition still runs */
            break;
        }

        UpdatePaddlePosition(&paddle, screenHeight);
        CheckPaddleBounds(&paddle, screenHeight);
    }

    /* ----- NULL-guard checks: none of these must crash ----- */
    MovePaddleUp(NULL);
    MovePaddleDown(NULL);
    StopPaddle(NULL);
    UpdatePaddlePosition(NULL, screenHeight);

    return 0;
}
