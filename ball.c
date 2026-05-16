/* =========================================================================
    Purple
    https://github.com/octopusnz/purple
    Copyright (c) 2026 Jacob Doherty
    SPDX-License-Identifier: MIT
    File: ball.c
    Description: Ball physics and collision detection
========================================================================= */

#include "ball.h"
#include <stddef.h>

#define SPIN_EFFECT_MULTIPLIER 3.0f
#define COLLISION_PUSHBACK 2.0f
/* Cap vertical speed to prevent the ball going nearly vertical after many
 * spin-accumulating bounces.  Chosen as 3× the maximum initial Y speed
 * (BALL_INITIAL_SPEED_Y 2.0 × max multiplier ~1.2 × 3 ≈ 7.2, rounded up).
 */
#define MAX_BALL_SPEED_Y 15.0f

void UpdateBallPosition(Ball* ball) {
    if (ball == NULL) return;
    
    ball->position.x += ball->velocity.x;
    ball->position.y += ball->velocity.y;
}

int IsCollidingVertical(const Ball* ball, int screenHeight) {
    if (ball == NULL) return 0;
    
    return (ball->position.y + ball->radius >= (float)screenHeight) ||
           (ball->position.y - ball->radius <= 0.0f);
}

// Check collision between ball and paddle (rectangle)
static int IsCollidingPaddle(const Ball* ball, Vector2 paddlePosition,
                              float paddleWidth, float paddleHeight)
{
    if (ball == NULL) return 0;

    // Calculate closest point on paddle to ball center
    float closestX = ball->position.x;
    float closestY = ball->position.y;

    if (ball->position.x < paddlePosition.x) {
        closestX = paddlePosition.x;
    } else if (ball->position.x > paddlePosition.x + paddleWidth) {
        closestX = paddlePosition.x + paddleWidth;
    }

    if (ball->position.y < paddlePosition.y) {
        closestY = paddlePosition.y;
    } else if (ball->position.y > paddlePosition.y + paddleHeight) {
        closestY = paddlePosition.y + paddleHeight;
    }

    // Calculate distance between ball center and closest point
    float dx = ball->position.x - closestX;
    float dy = ball->position.y - closestY;
    float distanceSquared = dx * dx + dy * dy;

    return distanceSquared < (ball->radius * ball->radius);
}

// Handle paddle collision and deflect ball
void HandlePaddleCollision(Ball* ball, Vector2 paddlePosition,
                           float paddleWidth, float paddleHeight)
{
    if (ball == NULL) return;

    if (!IsCollidingPaddle(ball, paddlePosition, paddleWidth, paddleHeight)) {
        return;
    }

    // Always reverse horizontal velocity on paddle collision
    ball->velocity.x *= -1.0f;

    // Push ball out of collision to prevent sticking
    if (ball->velocity.x > 0.0f) {
        // Ball moving right, push to right edge of paddle
        ball->position.x = paddlePosition.x + paddleWidth + ball->radius + COLLISION_PUSHBACK;
    } else {
        // Ball moving left, push to left edge of paddle
        ball->position.x = paddlePosition.x - ball->radius - COLLISION_PUSHBACK;
    }

    // Add spin based on where ball hits paddle (top/bottom adds vertical velocity)
    // Guard against division by zero for very small paddles
    float halfHeight = paddleHeight / 2.0f;
    if (halfHeight > 0.01f) {
        float hitPosition = ball->position.y - (paddlePosition.y + halfHeight);
        float spinFactor = hitPosition / halfHeight;  // Range: -1 to 1
        ball->velocity.y += spinFactor * SPIN_EFFECT_MULTIPLIER;
        // Clamp to prevent unbounded growth from repeated spin accumulation
        if (ball->velocity.y > MAX_BALL_SPEED_Y) ball->velocity.y = MAX_BALL_SPEED_Y;
        else if (ball->velocity.y < -MAX_BALL_SPEED_Y) ball->velocity.y = -MAX_BALL_SPEED_Y;
    }
}
