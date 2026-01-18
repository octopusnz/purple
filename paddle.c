/* =========================================================================
    Purple
    https://github.com/octopusnz/purple
    Copyright (c) 2026 Jacob Doherty
    SPDX-License-Identifier: MIT
    File: paddle.c
    Description: Paddle movement and AI logic
========================================================================= */

#include "paddle.h"
#include <stddef.h>

#define PADDLE_SPEED 6.0f
#define AI_SPEED_FACTOR 0.85f
#define AI_DEAD_ZONE 10.0f

void UpdatePaddlePosition(Paddle *paddle, int screenHeight)
{
    if (paddle == NULL) return;

    paddle->position.y += paddle->velocity;

    // Keep paddle within screen bounds
    if (paddle->position.y < 0.0f) {
        paddle->position.y = 0.0f;
    }
    
    // Calculate max Y position ensuring paddle height doesn't exceed screen
    float maxY = (float)screenHeight - paddle->height;
    if (maxY < 0.0f) {
        // Paddle is larger than screen, clamp to top
        maxY = 0.0f;
    }
    
    if (paddle->position.y > maxY) {
        paddle->position.y = maxY;
    }
}

void MovePaddleUp(Paddle *paddle)
{
    if (paddle == NULL) return;
    paddle->velocity = -PADDLE_SPEED;
}

void MovePaddleDown(Paddle *paddle)
{
    if (paddle == NULL) return;
    paddle->velocity = PADDLE_SPEED;
}

void StopPaddle(Paddle *paddle)
{
    if (paddle == NULL) return;
    paddle->velocity = 0.0f;
}

void UpdateAIPaddle(Paddle *paddle, Vector2 ballPosition, float ballRadius, int screenHeight)
{
    if (paddle == NULL) return;
    (void)ballRadius;  // Unused but kept for API consistency

    // Calculate paddle center
    float paddleCenter = paddle->position.y + paddle->height / 2.0f;

    // Move paddle towards ball with slight delay (imperfect AI)
    float aiSpeed = PADDLE_SPEED * AI_SPEED_FACTOR;

    if (ballPosition.y < paddleCenter - AI_DEAD_ZONE) {
        paddle->velocity = -aiSpeed;
    } else if (ballPosition.y > paddleCenter + AI_DEAD_ZONE) {
        paddle->velocity = aiSpeed;
    } else {
        paddle->velocity = 0.0f;
    }

    // Update position
    UpdatePaddlePosition(paddle, screenHeight);
}
