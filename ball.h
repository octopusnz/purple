/* =========================================================================
    Purple
    https://github.com/octopusnz/purple
    Copyright (c) 2026 Jacob Doherty
    SPDX-License-Identifier: MIT
    File: ball.h
    Description: Ball physics and collision detection
========================================================================= */

#ifndef BALL_H
#define BALL_H

#include <raylib/raylib.h>

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float radius;
} Ball;

// Update ball position based on velocity
void UpdateBallPosition(Ball* ball);

// Check if ball is colliding with top or bottom wall
int IsCollidingVertical(const Ball* ball, int screenHeight);

// Handle paddle collision and deflect ball with spin
void HandlePaddleCollision(Ball* ball, Vector2 paddlePosition, float paddleWidth, float paddleHeight);

#endif // BALL_H
