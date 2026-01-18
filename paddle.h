/* =========================================================================
    Purple
    https://github.com/octopusnz/purple
    Copyright (c) 2026 Jacob Doherty
    SPDX-License-Identifier: MIT
    See LICENSE.txt for 3rd party library and other resource licenses.
    File: paddle.h
    Description: Paddle movement and AI logic
========================================================================= */

#ifndef PADDLE_H
#define PADDLE_H

#include <raylib/raylib.h>

typedef struct {
    Vector2 position;  // Top-left corner of paddle
    float width;
    float height;
    float velocity;    // Current movement speed
    int score;
} Paddle;

// Update paddle position based on velocity
void UpdatePaddlePosition(Paddle *paddle, int screenHeight);

// Move paddle up (negative velocity)
void MovePaddleUp(Paddle *paddle);

// Move paddle down (positive velocity)
void MovePaddleDown(Paddle *paddle);

// Stop paddle movement
void StopPaddle(Paddle *paddle);

// AI logic: move paddle towards ball
void UpdateAIPaddle(Paddle *paddle, Vector2 ballPosition, float ballRadius, int screenHeight);

#endif // PADDLE_H
