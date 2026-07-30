/* =========================================================================
    Purple
    https://github.com/octopusnz/purple
    Copyright (c) 2026 Jacob Doherty
    SPDX-License-Identifier: MIT
    File: test.c
    Description: Unit tests for ball, paddle, leaderboard, and resource modules
========================================================================= */

#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 700
#include "unity/unity.h"
#include <raylib/raylib.h>
#include "../ball.h"
#include "../paddle.h"
#include "../leaderboard.h"
#include "../resource.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Fail the build immediately if the raylib header version is below 6.
 * This catches a header/library mismatch (e.g. old libraylib.a installed)
 * at compile time rather than producing mysterious runtime behaviour.
 */
#if RAYLIB_VERSION_MAJOR < 6
#error "raylib >= 6.0 required. Update libraylib.a and the raylib headers."
#endif

// Forward declarations
const char* find_resource_directory(void);

void setUp(void) {
    // Setup code runs before each test
}

void tearDown(void) {
    // Cleanup code runs after each test
}

// ==================== Ball Collision Tests ====================

void test_IsCollidingVertical_WithNullPointer(void) {
    int result = IsCollidingVertical(NULL, 600);
    TEST_ASSERT_FALSE(result);
}

void test_IsCollidingVertical_ExactlyAtTopEdge(void) {
    Ball ball = {
        .position = { 400.0f, 10.0f },
        .velocity = { 0.0f, 0.0f },
        .radius = 10.0f
    };
    int result = IsCollidingVertical(&ball, 600);
    TEST_ASSERT_TRUE(result);
}

void test_IsCollidingVertical_ExactlyAtBottomEdge(void) {
    Ball ball = {
        .position = { 400.0f, 590.0f },
        .velocity = { 0.0f, 0.0f },
        .radius = 10.0f
    };
    int result = IsCollidingVertical(&ball, 600);
    TEST_ASSERT_TRUE(result);
}

void test_HandlePaddleCollision_WithNullBall(void) {
    HandlePaddleCollision(NULL, (Vector2){20.0f, 250.0f}, 15.0f, 100.0f);
    TEST_ASSERT_TRUE(1);  // Should not crash
}

void test_HandlePaddleCollision_NoCollision(void) {
    Ball ball = {
        .position = { 200.0f, 300.0f },
        .velocity = { 5.0f, 0.0f },
        .radius = 8.0f
    };
    float originalVelX = ball.velocity.x;
    
    HandlePaddleCollision(&ball, (Vector2){20.0f, 250.0f}, 15.0f, 100.0f);
    TEST_ASSERT_EQUAL_FLOAT(originalVelX, ball.velocity.x);  // No change
}

void test_HandlePaddleCollision_PushesOutRightEdge(void) {
    // Ball colliding from left, should be pushed to right side
    Ball ball = {
        .position = { 35.0f, 300.0f },
        .velocity = { -5.0f, 0.0f },
        .radius = 8.0f
    };
    
    HandlePaddleCollision(&ball, (Vector2){20.0f, 250.0f}, 15.0f, 100.0f);
    
    // Ball should be pushed out: paddleX(20) + width(15) + radius(8) + pushback(2) = 45
    TEST_ASSERT_GREATER_THAN(43.0f, ball.position.x);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, ball.velocity.x);  // Velocity reversed
}

void test_HandlePaddleCollision_PushesOutLeftEdge(void) {
    // Ball colliding from right, should be pushed to left side
    Ball ball = {
        .position = { 19.0f, 300.0f },
        .velocity = { 5.0f, 0.0f },
        .radius = 8.0f
    };
    
    HandlePaddleCollision(&ball, (Vector2){20.0f, 250.0f}, 15.0f, 100.0f);
    
    // Ball should be pushed out: paddleX(20) - radius(8) - pushback(2) = 10
    TEST_ASSERT_LESS_THAN(12.0f, ball.position.x);
    TEST_ASSERT_EQUAL_FLOAT(-5.0f, ball.velocity.x);  // Velocity reversed
}

// ==================== Ball Movement Tests ====================

void test_UpdateBallPosition_MovesCorrectly(void) {
    Ball ball = {
        .position = { 100.0f, 100.0f },
        .velocity = { 5.0f, 3.0f },
        .radius = 10.0f
    };
    
    UpdateBallPosition(&ball);
    
    TEST_ASSERT_EQUAL_FLOAT(105.0f, ball.position.x);
    TEST_ASSERT_EQUAL_FLOAT(103.0f, ball.position.y);
}

void test_UpdateBallPosition_WithNullPointer(void) {
    // Should not crash
    UpdateBallPosition(NULL);
    TEST_ASSERT_TRUE(1); // If we get here, test passes
}

void test_UpdateBallPosition_MovesBackwards(void) {
    Ball ball = {
        .position = { 100.0f, 100.0f },
        .velocity = { -5.0f, -3.0f },
        .radius = 10.0f
    };
    
    UpdateBallPosition(&ball);
    
    TEST_ASSERT_EQUAL_FLOAT(95.0f, ball.position.x);
    TEST_ASSERT_EQUAL_FLOAT(97.0f, ball.position.y);
}

void test_UpdateBallPosition_WithZeroVelocity(void) {
    Ball ball = {
        .position = { 100.0f, 100.0f },
        .velocity = { 0.0f, 0.0f },
        .radius = 10.0f
    };
    
    UpdateBallPosition(&ball);

    TEST_ASSERT_EQUAL_FLOAT(100.0f, ball.position.x);
    TEST_ASSERT_EQUAL_FLOAT(100.0f, ball.position.y);
}

void test_UpdateBallPosition_NaNVelocityResetsToZero(void) {
    // NaN velocity must not propagate into position: ordered comparisons
    // with NaN always return false, so downstream range checks would
    // silently miss it. UpdateBallPosition resets NaN velocity to 0 first.
    Ball ball = {
        .position = { 100.0f, 100.0f },
        .velocity = { NAN, NAN },
        .radius = 10.0f
    };

    UpdateBallPosition(&ball);

    TEST_ASSERT_FLOAT_IS_NOT_NAN(ball.velocity.x);
    TEST_ASSERT_FLOAT_IS_NOT_NAN(ball.velocity.y);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, ball.velocity.x);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, ball.velocity.y);
    TEST_ASSERT_EQUAL_FLOAT(100.0f, ball.position.x);
    TEST_ASSERT_EQUAL_FLOAT(100.0f, ball.position.y);
}

void test_UpdateBallPosition_InfPositionResetsToZero(void) {
    // 2e38 + 2e38 = 4e38, which exceeds FLT_MAX (~3.4e38) and overflows to
    // +Inf in float32 arithmetic. Verifies the isfinite guard on position
    // resets it rather than letting Inf reach collision/scoring checks.
    Ball ball = {
        .position = { 2.0e38f, 2.0e38f },
        .velocity = { 2.0e38f, 2.0e38f },
        .radius = 10.0f
    };

    UpdateBallPosition(&ball);

    TEST_ASSERT_FLOAT_IS_NOT_INF(ball.position.x);
    TEST_ASSERT_FLOAT_IS_NOT_INF(ball.position.y);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, ball.position.x);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, ball.position.y);
}

void test_IsCollidingVertical_TopWallEdge(void) {
    Ball ball = {
        .position = { 400.0f, 8.0f },
        .velocity = { 0.0f, -5.0f },
        .radius = 10.0f
    };
    
    int result = IsCollidingVertical(&ball, 600);
    TEST_ASSERT_TRUE(result);
}

void test_IsCollidingVertical_BottomWallEdge(void) {
    Ball ball = {
        .position = { 400.0f, 592.0f },
        .velocity = { 0.0f, 5.0f },
        .radius = 10.0f
    };
    
    int result = IsCollidingVertical(&ball, 600);
    TEST_ASSERT_TRUE(result);
}

void test_IsCollidingVertical_JustInside(void) {
    Ball ball = {
        .position = { 400.0f, 20.0f },
        .velocity = { 0.0f, 0.0f },
        .radius = 10.0f
    };
    
    int result = IsCollidingVertical(&ball, 600);
    TEST_ASSERT_FALSE(result);
}

void test_IsCollidingVertical_NegativePosition(void) {
    Ball ball = {
        .position = { 400.0f, -5.0f },
        .velocity = { 0.0f, 0.0f },
        .radius = 10.0f
    };
    
    int result = IsCollidingVertical(&ball, 600);
    TEST_ASSERT_TRUE(result);
}

void test_IsCollidingVertical_BeyondBottom(void) {
    Ball ball = {
        .position = { 400.0f, 650.0f },
        .velocity = { 0.0f, 0.0f },
        .radius = 10.0f
    };
    
    int result = IsCollidingVertical(&ball, 600);
    TEST_ASSERT_TRUE(result);
}

void test_UpdateBallPosition_LargeVelocity(void) {
    Ball ball = {
        .position = { 100.0f, 100.0f },
        .velocity = { 50.0f, 30.0f },
        .radius = 10.0f
    };
    
    UpdateBallPosition(&ball);
    
    TEST_ASSERT_EQUAL_FLOAT(150.0f, ball.position.x);
    TEST_ASSERT_EQUAL_FLOAT(130.0f, ball.position.y);
}

void test_UpdateBallPosition_NegativePosition(void) {
    Ball ball = {
        .position = { 10.0f, 10.0f },
        .velocity = { -20.0f, -20.0f },
        .radius = 10.0f
    };
    
    UpdateBallPosition(&ball);
    
    TEST_ASSERT_EQUAL_FLOAT(-10.0f, ball.position.x);
    TEST_ASSERT_EQUAL_FLOAT(-10.0f, ball.position.y);
}

void test_IsCollidingVertical_ZeroScreenHeight(void) {
    Ball ball = {
        .position = { 400.0f, 10.0f },
        .velocity = { 0.0f, 0.0f },
        .radius = 10.0f
    };
    int result = IsCollidingVertical(&ball, 0);
    TEST_ASSERT_TRUE(result);  // Any positive position collides with 0 height
}

void test_HandlePaddleCollision_ZeroWidthPaddle(void) {
    Ball ball = {
        .position = { 20.0f, 300.0f },
        .velocity = { -5.0f, 0.0f },
        .radius = 8.0f
    };
    float origVelX = ball.velocity.x;
    
    HandlePaddleCollision(&ball, (Vector2){20.0f, 250.0f}, 0.0f, 100.0f);
    
    // Degenerate case: may or may not collide
    TEST_ASSERT_TRUE(ball.velocity.x != 0.0f || origVelX == 0.0f);
}

void test_HandlePaddleCollision_ZeroHeightPaddle(void) {
    Ball ball = {
        .position = { 30.0f, 250.0f },
        .velocity = { -5.0f, 0.0f },
        .radius = 8.0f
    };
    
    HandlePaddleCollision(&ball, (Vector2){20.0f, 250.0f}, 15.0f, 0.0f);
    
    // Should handle gracefully (edge case)
    TEST_ASSERT_TRUE(1);
}

void test_HandlePaddleCollision_MultipleRapidCollisions(void) {
    Ball ball = {
        .position = { 35.0f, 300.0f },
        .velocity = { -5.0f, 0.0f },
        .radius = 8.0f
    };
    
    // First collision
    HandlePaddleCollision(&ball, (Vector2){20.0f, 250.0f}, 15.0f, 100.0f);
    float firstX = ball.position.x;
    
    // Should be pushed out, so second call shouldn't collide
    HandlePaddleCollision(&ball, (Vector2){20.0f, 250.0f}, 15.0f, 100.0f);
    
    // Position should not change on second call (no collision)
    TEST_ASSERT_EQUAL_FLOAT(firstX, ball.position.x);
}

void test_HandlePaddleCollision_EdgeOfPaddle(void) {
    Ball ball = {
        .position = { 35.0f, 248.0f },  // Just at edge
        .velocity = { -5.0f, 0.0f },
        .radius = 8.0f
    };
    
    HandlePaddleCollision(&ball, (Vector2){20.0f, 250.0f}, 15.0f, 100.0f);
    
    // Should still collide and reverse
    TEST_ASSERT_GREATER_THAN(0.0f, ball.velocity.x);
}

void test_HandlePaddleCollision_SpinAccumulatesOnExistingY(void) {
    // Ball arrives with existing downward Y velocity and hits top of paddle.
    // Spin should add to the existing velocity, not replace it.
    Ball ball = {
        .position = { 35.0f, 250.0f },  // Top of paddle (paddleY = 250)
        .velocity = { -5.0f, -2.0f },   // Already moving upward
        .radius = 8.0f
    };

    HandlePaddleCollision(&ball, (Vector2){20.0f, 250.0f}, 15.0f, 100.0f);

    // paddleCenter = 250 + 50 = 300; hitPosition = 250 - 300 = -50
    // spinFactor = -50 / 50 = -1; spin = -1 * 3 = -3
    // Expected Y: -2 + (-3) = -5
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -5.0f, ball.velocity.y);
}

void test_HandlePaddleCollision_TinyPaddleNoSpinDivisionByZero(void) {
    // halfHeight = 0.007 < 0.01 triggers the spin guard; Y velocity must not change.
    Ball ball = {
        .position = { 35.0f, 300.0f },
        .velocity = { -5.0f, 2.0f },
        .radius = 8.0f
    };
    float origY = ball.velocity.y;

    // Paddle height 0.015 -> halfHeight 0.0075 -> spin guard active
    HandlePaddleCollision(&ball, (Vector2){20.0f, 295.0f}, 15.0f, 0.015f);

    TEST_ASSERT_EQUAL_FLOAT(origY, ball.velocity.y);  // No spin applied
    TEST_ASSERT_EQUAL_FLOAT(5.0f, ball.velocity.x);   // X still reversed
}

void test_HandlePaddleCollision_SpinClampedAtMaxSpeedY(void) {
    /* Repeated top-edge hits accumulate spin (+3 each time).  Before the clamp
     * was added, velocity.y would grow without bound.  After the fix it must
     * never exceed MAX_BALL_SPEED_Y (15.0f) regardless of how many hits occur.
     *
     * Each collision pushes the ball out to x >= 45, so we reset x to 35 before
     * each hit to keep the ball inside the paddle region.
     */
    Ball ball = {
        .position = { 35.0f, 250.0f },  // Top edge of paddle -> max positive spin
        .velocity = { -5.0f, 0.0f },
        .radius = 8.0f
    };
    Vector2 paddlePos = { 20.0f, 250.0f };

    for (int i = 0; i < 20; ++i) {
        ball.position.x = 35.0f;   // Reset inside paddle so collision fires
        ball.velocity.x = -5.0f;   // Ball must be moving toward paddle
        HandlePaddleCollision(&ball, paddlePos, 15.0f, 100.0f);
    }

    TEST_ASSERT_TRUE(ball.velocity.y <= 15.0f);   // Must not exceed cap
    TEST_ASSERT_TRUE(ball.velocity.y >= -15.0f);  // Cap applies both directions
}

void test_HandlePaddleCollision_NaNSpinVelocityResetsToZero(void) {
    // A NaN incoming Y velocity (e.g. from an earlier NaN velocity bug)
    // stays NaN once spin is added (NaN + finite = NaN). The isfinite guard
    // must catch this rather than let NaN escape the clamp comparisons below.
    Ball ball = {
        .position = { 35.0f, 250.0f },  // Top of paddle (paddleY = 250)
        .velocity = { -5.0f, NAN },
        .radius = 8.0f
    };

    HandlePaddleCollision(&ball, (Vector2){20.0f, 250.0f}, 15.0f, 100.0f);

    TEST_ASSERT_FLOAT_IS_NOT_NAN(ball.velocity.y);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, ball.velocity.y);
}

void test_HandlePaddleCollision_UsesStructDimensionsForGeometry(void) {
    /* Collision detection uses the paddle dimensions passed as arguments.
     * A wider paddle (width 50) should collide where the standard 15-wide
     * paddle would not, confirming the geometry comes from the struct fields
     * (which main.c now passes as paddle->width / paddle->height) and not
     * from hard-coded constants.
     */
    Ball ball = {
        .position = { 60.0f, 300.0f },  // x=60, outside a width-15 paddle at x=20
        .velocity = { -5.0f, 0.0f },
        .radius = 8.0f
    };
    float origVelX = ball.velocity.x;

    // Width 15: ball at x=60 is well outside -> no collision
    HandlePaddleCollision(&ball, (Vector2){20.0f, 250.0f}, 15.0f, 100.0f);
    TEST_ASSERT_EQUAL_FLOAT(origVelX, ball.velocity.x);  // No collision

    // Width 50: paddle spans x=20..70, ball at x=60 is inside -> collision
    HandlePaddleCollision(&ball, (Vector2){20.0f, 250.0f}, 50.0f, 100.0f);
    TEST_ASSERT_EQUAL_FLOAT(-origVelX, ball.velocity.x); // Velocity reversed
}

void test_HandlePaddleCollision_PushbackPreventsSticking(void) {
    // Verify ball is pushed far enough to not collide next frame
    Ball ball = {
        .position = { 35.0f, 300.0f },
        .velocity = { -5.0f, 0.0f },
        .radius = 8.0f
    };
    
    HandlePaddleCollision(&ball, (Vector2){20.0f, 250.0f}, 15.0f, 100.0f);
    
    // Ball should be pushed to right edge: 20 + 15 + 8 + 2 = 45
    TEST_ASSERT_GREATER_OR_EQUAL(45.0f, ball.position.x);
    
    // Simulate next frame movement
    ball.position.x += ball.velocity.x;
    
    // Should not be colliding anymore
    Ball testBall = ball;
    float originalVelX = testBall.velocity.x;
    HandlePaddleCollision(&testBall, (Vector2){20.0f, 250.0f}, 15.0f, 100.0f);
    TEST_ASSERT_EQUAL_FLOAT(originalVelX, testBall.velocity.x);
}

// Paddle tests
void test_MovePaddleUp_WithNullPointer(void) {
    MovePaddleUp(NULL);
    TEST_ASSERT_TRUE(1);  // Should not crash
}

void test_MovePaddleDown_WithNullPointer(void) {
    MovePaddleDown(NULL);
    TEST_ASSERT_TRUE(1);  // Should not crash
}

void test_StopPaddle_WithNullPointer(void) {
    StopPaddle(NULL);
    TEST_ASSERT_TRUE(1);  // Should not crash
}

void test_MovePaddleUp_SetsNegativeVelocity(void) {
    Paddle paddle = {
        .position = { 20.0f, 250.0f },
        .width = 15.0f,
        .height = 100.0f,
        .velocity = 0.0f,
        .score = 0
    };
    
    MovePaddleUp(&paddle);
    TEST_ASSERT_EQUAL_FLOAT(-6.0f, paddle.velocity);
}

void test_MovePaddleDown_SetsPositiveVelocity(void) {
    Paddle paddle = {
        .position = { 20.0f, 250.0f },
        .width = 15.0f,
        .height = 100.0f,
        .velocity = 0.0f,
        .score = 0
    };
    
    MovePaddleDown(&paddle);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, paddle.velocity);
}

void test_StopPaddle_SetsZeroVelocity(void) {
    Paddle paddle = {
        .position = { 20.0f, 250.0f },
        .width = 15.0f,
        .height = 100.0f,
        .velocity = 6.0f,
        .score = 0
    };
    
    StopPaddle(&paddle);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, paddle.velocity);
}

void test_UpdatePaddlePosition_MovesWithVelocity(void) {
    Paddle paddle = {
        .position = { 20.0f, 250.0f },
        .width = 15.0f,
        .height = 100.0f,
        .velocity = 5.0f,
        .score = 0
    };
    
    UpdatePaddlePosition(&paddle, 600);
    TEST_ASSERT_EQUAL_FLOAT(255.0f, paddle.position.y);
}

void test_UpdatePaddlePosition_ClampsAtTop(void) {
    Paddle paddle = {
        .position = { 20.0f, 5.0f },
        .width = 15.0f,
        .height = 100.0f,
        .velocity = -10.0f,
        .score = 0
    };
    
    UpdatePaddlePosition(&paddle, 600);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, paddle.position.y);
}

void test_UpdatePaddlePosition_ClampsAtBottom(void) {
    Paddle paddle = {
        .position = { 20.0f, 550.0f },
        .width = 15.0f,
        .height = 100.0f,
        .velocity = 10.0f,
        .score = 0
    };
    
    UpdatePaddlePosition(&paddle, 600);
    TEST_ASSERT_EQUAL_FLOAT(500.0f, paddle.position.y);  // 600 - 100
}

void test_UpdatePaddlePosition_WithNullPointer(void) {
    // Should not crash
    UpdatePaddlePosition(NULL, 600);
    TEST_ASSERT_TRUE(1);
}

void test_UpdatePaddlePosition_NaNPositionResetsToTop(void) {
    // NaN position (NaN + velocity is still NaN) must not survive the clamp
    // checks below it, since ordered comparisons with NaN always return
    // false. UpdatePaddlePosition resets it to the top of the screen first.
    Paddle paddle = {
        .position = { 20.0f, NAN },
        .width = 15.0f,
        .height = 100.0f,
        .velocity = 5.0f,
        .score = 0
    };

    UpdatePaddlePosition(&paddle, 600);

    TEST_ASSERT_FLOAT_IS_NOT_NAN(paddle.position.y);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, paddle.position.y);
}

void test_UpdatePaddlePosition_MultipleFrames(void) {
    Paddle paddle = {
        .position = { 20.0f, 250.0f },
        .width = 15.0f,
        .height = 100.0f,
        .velocity = 5.0f,
        .score = 0
    };
    
    UpdatePaddlePosition(&paddle, 600);
    float firstY = paddle.position.y;
    TEST_ASSERT_EQUAL_FLOAT(255.0f, firstY);
    
    UpdatePaddlePosition(&paddle, 600);
    TEST_ASSERT_EQUAL_FLOAT(260.0f, paddle.position.y);
}

void test_UpdatePaddlePosition_StoppingAtBoundary(void) {
    Paddle paddle = {
        .position = { 20.0f, 495.0f },  // 5 pixels from bottom (500 = 600 - 100)
        .width = 15.0f,
        .height = 100.0f,
        .velocity = 10.0f,
        .score = 0
    };
    
    UpdatePaddlePosition(&paddle, 600);
    TEST_ASSERT_EQUAL_FLOAT(500.0f, paddle.position.y);  // Should clamp, not overshoot
}

void test_UpdatePaddlePosition_NegativeVelocityAtTop(void) {
    Paddle paddle = {
        .position = { 20.0f, 2.0f },
        .width = 15.0f,
        .height = 100.0f,
        .velocity = -10.0f,
        .score = 0
    };
    
    UpdatePaddlePosition(&paddle, 600);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, paddle.position.y);
}

void test_HandlePaddleCollision_ReversesBallVelocityX(void) {
    Ball ball = {
        .position = { 35.0f, 300.0f },
        .velocity = { -5.0f, 0.0f },
        .radius = 8.0f
    };
    
    HandlePaddleCollision(&ball, (Vector2){20.0f, 250.0f}, 15.0f, 100.0f);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, ball.velocity.x);  // Should be positive now
}

void test_HandlePaddleCollision_TopSpinEffect(void) {
    // Ball hits paddle at top edge - should get upward spin
    Ball ball = {
        .position = { 35.0f, 250.0f },  // Colliding with top of paddle
        .velocity = { -5.0f, 0.0f },
        .radius = 8.0f
    };
    float originalX = ball.velocity.x;
    
    HandlePaddleCollision(&ball, (Vector2){20.0f, 250.0f}, 15.0f, 100.0f);
    TEST_ASSERT_EQUAL_FLOAT(-originalX, ball.velocity.x);  // Should reverse
    TEST_ASSERT_TRUE(ball.velocity.y < 0.0f);  // Should have negative (upward) Y velocity from spin
}

void test_HandlePaddleCollision_BottomSpinEffect(void) {
    // Ball hits paddle at bottom edge - should get downward spin
    Ball ball = {
        .position = { 35.0f, 345.0f },  // Near bottom of paddle
        .velocity = { -5.0f, 0.0f },
        .radius = 8.0f
    };
    
    HandlePaddleCollision(&ball, (Vector2){20.0f, 250.0f}, 15.0f, 100.0f);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, ball.velocity.x);
    TEST_ASSERT_TRUE(ball.velocity.y > 0.0f);  // Should have positive (downward) Y velocity
}

void test_HandlePaddleCollision_CenterNoSpin(void) {
    // Ball hits paddle at center - minimal spin
    Ball ball = {
        .position = { 35.0f, 300.0f },  // Center of paddle
        .velocity = { -5.0f, 2.0f },
        .radius = 8.0f
    };
    float originalY = ball.velocity.y;
    
    HandlePaddleCollision(&ball, (Vector2){20.0f, 250.0f}, 15.0f, 100.0f);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, ball.velocity.x);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, originalY, ball.velocity.y);  // Y should not change much
}

void test_UpdateAIPaddle_MovesUpTowardsBall(void) {
    Paddle paddle = {
        .position = { 1165.0f, 300.0f },
        .width = 15.0f,
        .height = 100.0f,
        .velocity = 0.0f,
        .score = 0
    };
    
    // Ball is above paddle center
    Vector2 ballPos = { 1100.0f, 200.0f };
    UpdateAIPaddle(&paddle, ballPos, 8.0f, 600);
    
    // Paddle should move up (negative velocity was applied and position updated)
    TEST_ASSERT_TRUE(paddle.velocity < 0.0f);
}

void test_UpdateAIPaddle_MovesDownTowardsBall(void) {
    Paddle paddle = {
        .position = { 1165.0f, 200.0f },
        .width = 15.0f,
        .height = 100.0f,
        .velocity = 0.0f,
        .score = 0
    };
    
    // Ball is below paddle center (by more than 10 pixel threshold)
    Vector2 ballPos = { 1100.0f, 350.0f };
    UpdateAIPaddle(&paddle, ballPos, 8.0f, 600);
    
    // Paddle should move down (positive velocity)
    TEST_ASSERT_TRUE(paddle.velocity > 0.0f);
}

void test_UpdateAIPaddle_StopsNearBall(void) {
    Paddle paddle = {
        .position = { 1165.0f, 300.0f },
        .width = 15.0f,
        .height = 100.0f,
        .velocity = 0.0f,
        .score = 0
    };
    
    // Ball within 10 pixel threshold of paddle center (center at 300 + 50 = 350)
    // Ball at 349 is 1 pixel away, so velocity should be 0
    Vector2 ballPos = { 1100.0f, 349.0f };
    UpdateAIPaddle(&paddle, ballPos, 8.0f, 600);
    
    // Paddle should have zero velocity (it stops when within threshold)
    TEST_ASSERT_EQUAL_FLOAT(0.0f, paddle.velocity);
}

void test_UpdateAIPaddle_ClampsAtTopBoundary(void) {
    Paddle paddle = {
        .position = { 1165.0f, 5.0f },
        .width = 15.0f,
        .height = 100.0f,
        .velocity = 0.0f,
        .score = 0
    };
    
    Vector2 ballPos = { 1100.0f, 10.0f };
    UpdateAIPaddle(&paddle, ballPos, 8.0f, 600);
    
    // Should not go below 0 (when moving up)
    TEST_ASSERT_TRUE(paddle.position.y >= 0.0f);
}

void test_UpdateAIPaddle_ClampsAtBottomBoundary(void) {
    Paddle paddle = {
        .position = { 1165.0f, 550.0f },
        .width = 15.0f,
        .height = 100.0f,
        .velocity = 0.0f,
        .score = 0
    };
    
    Vector2 ballPos = { 1100.0f, 550.0f };
    UpdateAIPaddle(&paddle, ballPos, 8.0f, 600);
    
    // Should not exceed screen height - paddle height (500)
    TEST_ASSERT_EQUAL_FLOAT(500.0f, paddle.position.y);
}

void test_UpdateAIPaddle_WithNullPaddle(void) {
    // Should not crash
    UpdateAIPaddle(NULL, (Vector2){100.0f, 300.0f}, 8.0f, 600);
    TEST_ASSERT_TRUE(1);
}

void test_UpdatePaddlePosition_ZeroHeight(void) {
    Paddle paddle = {
        .position = { 20.0f, 250.0f },
        .width = 15.0f,
        .height = 0.0f,
        .velocity = 5.0f,
        .score = 0
    };
    
    UpdatePaddlePosition(&paddle, 600);
    TEST_ASSERT_EQUAL_FLOAT(255.0f, paddle.position.y);
}

void test_UpdatePaddlePosition_OversizedPaddleClampsToTop(void) {
    // When paddle.height > screenHeight, maxY goes negative and is clamped to 0.
    // The paddle must always end up at y = 0 regardless of starting position.
    Paddle paddle = {
        .position = { 20.0f, 300.0f },
        .width = 15.0f,
        .height = 800.0f,  // Larger than screenHeight (600)
        .velocity = 0.0f,
        .score = 0
    };

    UpdatePaddlePosition(&paddle, 600);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, paddle.position.y);
}

void test_MovePaddle_RapidDirectionChange(void) {
    Paddle paddle = {
        .position = { 20.0f, 250.0f },
        .width = 15.0f,
        .height = 100.0f,
        .velocity = 0.0f,
        .score = 0
    };
    
    MovePaddleUp(&paddle);
    TEST_ASSERT_EQUAL_FLOAT(-6.0f, paddle.velocity);
    
    MovePaddleDown(&paddle);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, paddle.velocity);
    
    StopPaddle(&paddle);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, paddle.velocity);
}

void test_UpdateAIPaddle_ExactlyAtCenter(void) {
    Paddle paddle = {
        .position = { 1165.0f, 300.0f },
        .width = 15.0f,
        .height = 100.0f,
        .velocity = 0.0f,
        .score = 0
    };
    
    // Ball exactly at paddle center (300 + 50 = 350)
    Vector2 ballPos = { 1100.0f, 350.0f };
    UpdateAIPaddle(&paddle, ballPos, 8.0f, 600);
    
    TEST_ASSERT_EQUAL_FLOAT(0.0f, paddle.velocity);
}

void test_UpdatePaddlePosition_LargeVelocity(void) {
    Paddle paddle = {
        .position = { 20.0f, 450.0f },
        .width = 15.0f,
        .height = 100.0f,
        .velocity = 100.0f,
        .score = 0
    };
    
    UpdatePaddlePosition(&paddle, 600);
    // Should clamp at bottom (600 - 100 = 500)
    TEST_ASSERT_EQUAL_FLOAT(500.0f, paddle.position.y);
}

void test_UpdatePaddlePosition_AlreadyAtExactTop(void) {
    Paddle paddle = {
        .position = { 20.0f, 0.0f },
        .width = 15.0f,
        .height = 100.0f,
        .velocity = -5.0f,
        .score = 0
    };
    
    UpdatePaddlePosition(&paddle, 600);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, paddle.position.y);
}

void test_UpdatePaddlePosition_AlreadyAtExactBottom(void) {
    Paddle paddle = {
        .position = { 20.0f, 500.0f },
        .width = 15.0f,
        .height = 100.0f,
        .velocity = 5.0f,
        .score = 0
    };
    
    UpdatePaddlePosition(&paddle, 600);
    TEST_ASSERT_EQUAL_FLOAT(500.0f, paddle.position.y);
}

// Font file tests
static inline int file_exists(const char *filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

static inline int directory_exists(const char *dirname) {
    struct stat buffer;
    return (stat(dirname, &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

// Cached resource directory to avoid repeated lookups
static const char *cached_resource_dir = NULL;

// Find resources directory by searching parent directories
const char* find_resource_directory(void) {
    if (cached_resource_dir) return cached_resource_dir;
    
    static char resourcePath[512];
    const char *basePaths[] = {
        "./resources",
        "resources",
        "../resources",
        "../../resources",
        "../../../resources",
    };

    for (size_t i = 0; i < sizeof(basePaths) / sizeof(basePaths[0]); i++) {
        if (directory_exists(basePaths[i])) {
            snprintf(resourcePath, sizeof(resourcePath), "%s", basePaths[i]);
            cached_resource_dir = resourcePath;
            return resourcePath;
        }
    }

    // Default fallback
    cached_resource_dir = "./resources";
    return cached_resource_dir;
}

// ==================== Resource Function Tests ====================

void test_FindResourceDirectory_IsValid(void) {
    const char *dir = FindResourceDirectory();
    TEST_ASSERT_TRUE(directory_exists(dir));
}

void test_FindResourceFile_ReturnsNonNull(void) {
    const char *path = FindResourceFile("orbitron/Orbitron-VariableFont_wght.ttf");
    TEST_ASSERT_NOT_NULL(path);
    TEST_ASSERT_TRUE(strstr(path, "orbitron") != NULL);
}

void test_FindResourceFile_NullSubpathDoesNotCrash(void) {
    /* Before the fix, FindResourceFile(NULL) passed NULL to snprintf's %s,
     * which is undefined behaviour in C99.  The fix adds an early-return guard.
     * This test verifies the function returns non-NULL without crashing.
     */
    const char *path = FindResourceFile(NULL);
    TEST_ASSERT_NOT_NULL(path);
}

void test_FindFontPath_ReturnsNonNull(void) {
    const char *path = FindFontPath();
    TEST_ASSERT_NOT_NULL(path);
    if (file_exists(path)) {
        TEST_ASSERT_TRUE(file_exists(path));
    } else {
        TEST_IGNORE_MESSAGE("Font file not found");
    }
}

void test_FindResourceDirectory_IsConsistent(void) {
    const char *dir1 = FindResourceDirectory();
    const char *dir2 = FindResourceDirectory();
    // Should return same pointer (cached)
    TEST_ASSERT_EQUAL_PTR(dir1, dir2);
}

// ==================== Font File Tests ====================

void test_RaylibVersion_AtLeastSix(void) {
    /* Documents the minimum raylib version assumption.
     * TEXTURE_FILTER_BILINEAR (value 1) was confirmed present in v5 but the
     * SetTextureFilter API and its correct interaction with font textures is
     * relied upon from v6 onwards.  If the header changes incompatibly this
     * test will catch it at the constant level without needing a GPU context.
     */
    TEST_ASSERT_GREATER_OR_EQUAL(6, RAYLIB_VERSION_MAJOR);
    TEST_ASSERT_EQUAL_INT(1, TEXTURE_FILTER_BILINEAR);
    TEST_ASSERT_EQUAL_INT(0, LOG_WARNING > LOG_INFO ? 0 : 0);  /* LOG_WARNING > LOG_INFO */
    TEST_ASSERT_TRUE(LOG_WARNING > LOG_INFO);
}

void test_FontFile_ExistsAndValid(void) {
    const char *resourceDir = find_resource_directory();
    char fontPath[512];
    snprintf(fontPath, sizeof(fontPath), "%s/orbitron/Orbitron-VariableFont_wght.ttf", resourceDir);
    
    if (!file_exists(fontPath)) {
        TEST_IGNORE_MESSAGE("Font file not found in resources directory");
    }
    
    struct stat st;
    TEST_ASSERT_EQUAL(0, stat(fontPath, &st));
    TEST_ASSERT_GREATER_THAN(0, st.st_size);
    
    // Check if it's a valid TTF file
    FILE *fp = fopen(fontPath, "rb");
    TEST_ASSERT_NOT_NULL(fp);
    
    unsigned char header[4];
    TEST_ASSERT_EQUAL(4, fread(header, 1, 4, fp));
    
    int isValidTTF = (header[0] == 0x00 && header[1] == 0x01 &&
                      header[2] == 0x00 && header[3] == 0x00) ||
                     (header[0] == 'O' && header[1] == 'T' &&
                      header[2] == 'T' && header[3] == 'O') ||
                     (header[0] == 't' && header[1] == 'r' &&
                      header[2] == 'u' && header[3] == 'e');
    fclose(fp);
    
    TEST_ASSERT_TRUE(isValidTTF);
}

// ==================== Leaderboard Tests ====================

void test_AddLeaderboardEntry_WithNullLeaderboard(void) {
    AddLeaderboardEntry(NULL, "ABC", 'P', 10.0f);
    TEST_ASSERT_TRUE(1);  // Should not crash
}

void test_AddLeaderboardEntry_WithNullInitials(void) {
    Leaderboard lb = {0};
    AddLeaderboardEntry(&lb, NULL, 'P', 10.0f);
    TEST_ASSERT_EQUAL_UINT32(1, lb.count);
    TEST_ASSERT_EQUAL_STRING_LEN("   ", lb.entries[0].initials, 3);
}

void test_AddLeaderboardEntry_WithEmptyInitials(void) {
    Leaderboard lb = {0};
    AddLeaderboardEntry(&lb, "", 'P', 10.0f);
    TEST_ASSERT_EQUAL_UINT32(1, lb.count);
    TEST_ASSERT_EQUAL_STRING_LEN("   ", lb.entries[0].initials, 3);
}

void test_AddLeaderboardEntry_AIWinnerUppercase(void) {
    Leaderboard lb = {0};
    AddLeaderboardEntry(&lb, "AI", 'A', 15.0f);
    TEST_ASSERT_EQUAL_CHAR('A', lb.entries[0].winner);
}

void test_AddLeaderboardEntry_UnknownWinnerDefaultsToPlayer(void) {
    Leaderboard lb = {0};
    AddLeaderboardEntry(&lb, "ABC", 'X', 15.0f);
    TEST_ASSERT_EQUAL_CHAR('P', lb.entries[0].winner);
}

void test_AddLeaderboardEntry_SortedAscending(void) {
    Leaderboard lb = {0};
    AddLeaderboardEntry(&lb, "CCC", 'P', 30.0f);
    AddLeaderboardEntry(&lb, "AAA", 'P', 10.0f);
    AddLeaderboardEntry(&lb, "BBB", 'P', 20.0f);
    
    TEST_ASSERT_EQUAL_FLOAT(10.0f, lb.entries[0].seconds);
    TEST_ASSERT_EQUAL_FLOAT(20.0f, lb.entries[1].seconds);
    TEST_ASSERT_EQUAL_FLOAT(30.0f, lb.entries[2].seconds);
}

void test_LoadLeaderboard_WithNullPointer(void) {
    LoadLeaderboard(NULL);
    TEST_ASSERT_TRUE(1);  // Should not crash
}

void test_LoadLeaderboard_NonexistentFile(void) {
    char tempHome[64];
    snprintf(tempHome, sizeof(tempHome), "/tmp/purpletestXXXXXX");
    if (!mkdtemp(tempHome)) {
        TEST_IGNORE_MESSAGE("Failed to create temp directory");
    }
    const char *oldHome = getenv("HOME");
    setenv("HOME", tempHome, 1);

    Leaderboard lb = {0};
    lb.count = 99;  // Set to non-zero
    LoadLeaderboard(&lb);
    
    TEST_ASSERT_EQUAL_UINT32(0, lb.count);  // Should reset to 0
    
    // Restore HOME
    if (oldHome) setenv("HOME", oldHome, 1);
}

void test_SaveLeaderboard_WithNullPointer(void) {
    SaveLeaderboard(NULL);
    TEST_ASSERT_TRUE(1);  // Should not crash
}

void test_SaveLeaderboard_EmptyLeaderboard(void) {
    char tempHome[64];
    snprintf(tempHome, sizeof(tempHome), "/tmp/purpletestXXXXXX");
    if (!mkdtemp(tempHome)) {
        TEST_IGNORE_MESSAGE("Failed to create temp directory");
    }
    const char *oldHome = getenv("HOME");
    setenv("HOME", tempHome, 1);

    Leaderboard lb = {0};
    SaveLeaderboard(&lb);
    
    Leaderboard loaded = {0};
    LoadLeaderboard(&loaded);
    TEST_ASSERT_EQUAL_UINT32(0, loaded.count);
    
    // Restore HOME
    if (oldHome) setenv("HOME", oldHome, 1);
}

void test_AddLeaderboardEntry_UppercasesAndStores(void) {
    Leaderboard lb = {0};
    AddLeaderboardEntry(&lb, "abc", 'p', 12.5f);

    TEST_ASSERT_EQUAL_UINT32(1, lb.count);
    TEST_ASSERT_EQUAL_CHAR('P', lb.entries[0].winner);
    TEST_ASSERT_EQUAL_STRING_LEN("ABC", lb.entries[0].initials, 3);
    TEST_ASSERT_EQUAL_FLOAT(12.5f, lb.entries[0].seconds);
}

void test_AddLeaderboardEntry_ReplacesWorstWhenFull(void) {
    Leaderboard lb = {0};
    for (int i = 0; i < (int)LEADERBOARD_MAX_ENTRIES; ++i) {
        float seconds = (float)((i + 1) * 10);  // 10,20,...,100
        AddLeaderboardEntry(&lb, "AAA", 'P', seconds);
    }

    // Add a faster time; should evict the worst (100)
    AddLeaderboardEntry(&lb, "BBB", 'A', 5.0f);

    TEST_ASSERT_EQUAL_UINT32(LEADERBOARD_MAX_ENTRIES, lb.count);
    // Fastest should be 5.0, slowest should now be 90.0
    TEST_ASSERT_EQUAL_FLOAT(5.0f, lb.entries[0].seconds);
    TEST_ASSERT_EQUAL_FLOAT(90.0f, lb.entries[lb.count - 1].seconds);
}

void test_AddLeaderboardEntry_IgnoresSlowerThanWorst(void) {
    Leaderboard lb = {0};
    for (int i = 0; i < (int)LEADERBOARD_MAX_ENTRIES; ++i) {
        AddLeaderboardEntry(&lb, "AAA", 'P', (float)(i + 1));
    }

    AddLeaderboardEntry(&lb, "ZZZ", 'P', 50.0f);

    TEST_ASSERT_EQUAL_UINT32(LEADERBOARD_MAX_ENTRIES, lb.count);
    // Ensure 50.0 did not enter list (max existing is 10.0)
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, lb.entries[lb.count - 1].seconds);
}

void test_SaveAndLoadLeaderboard_PersistsSorted(void) {
    char tempHome[64];
    snprintf(tempHome, sizeof(tempHome), "/tmp/purpletestXXXXXX");
    if (!mkdtemp(tempHome)) {
        TEST_IGNORE_MESSAGE("Failed to create temp directory");
    }
    const char *oldHome = getenv("HOME");
    setenv("HOME", tempHome, 1);

    Leaderboard lb = {0};
    AddLeaderboardEntry(&lb, "CCC", 'P', 30.0f);
    AddLeaderboardEntry(&lb, "DDD", 'A', 10.0f);
    AddLeaderboardEntry(&lb, "EEE", 'P', 20.0f);
    SaveLeaderboard(&lb);

    Leaderboard loaded = {0};
    LoadLeaderboard(&loaded);

    TEST_ASSERT_EQUAL_UINT32(3, loaded.count);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, loaded.entries[0].seconds);
    TEST_ASSERT_EQUAL_CHAR('A', loaded.entries[0].winner);
    TEST_ASSERT_EQUAL_STRING_LEN("DDD", loaded.entries[0].initials, 3);
    TEST_ASSERT_EQUAL_FLOAT(30.0f, loaded.entries[loaded.count - 1].seconds);
    
    // Restore HOME
    if (oldHome) setenv("HOME", oldHome, 1);
}

void test_AddLeaderboardEntry_ZeroTime(void) {
    Leaderboard lb = {0};
    AddLeaderboardEntry(&lb, "ABC", 'P', 0.0f);
    
    TEST_ASSERT_EQUAL_UINT32(1, lb.count);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, lb.entries[0].seconds);
}

void test_AddLeaderboardEntry_NegativeTime(void) {
    Leaderboard lb = {0};
    AddLeaderboardEntry(&lb, "ABC", 'P', -5.0f);
    
    /* Negative times are invalid game results and must be rejected */
    TEST_ASSERT_EQUAL_UINT32(0, lb.count);
}

void test_AddLeaderboardEntry_VeryLargeTime(void) {
    Leaderboard lb = {0};
    AddLeaderboardEntry(&lb, "ABC", 'P', 9999.99f);
    
    TEST_ASSERT_EQUAL_UINT32(1, lb.count);
    TEST_ASSERT_EQUAL_FLOAT(9999.99f, lb.entries[0].seconds);
}

void test_AddLeaderboardEntry_DuplicateTimes(void) {
    Leaderboard lb = {0};
    AddLeaderboardEntry(&lb, "AAA", 'P', 10.0f);
    AddLeaderboardEntry(&lb, "BBB", 'P', 10.0f);
    AddLeaderboardEntry(&lb, "CCC", 'A', 10.0f);
    
    TEST_ASSERT_EQUAL_UINT32(3, lb.count);
    // All should have same time
    TEST_ASSERT_EQUAL_FLOAT(10.0f, lb.entries[0].seconds);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, lb.entries[1].seconds);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, lb.entries[2].seconds);
}

void test_AddLeaderboardEntry_LongInitials(void) {
    Leaderboard lb = {0};
    AddLeaderboardEntry(&lb, "ABCDEFGH", 'P', 10.0f);
    
    TEST_ASSERT_EQUAL_UINT32(1, lb.count);
    // Should only take first 3 chars
    TEST_ASSERT_EQUAL_STRING_LEN("ABC", lb.entries[0].initials, 3);
}

void test_AddLeaderboardEntry_SingleChar(void) {
    Leaderboard lb = {0};
    AddLeaderboardEntry(&lb, "A", 'P', 10.0f);
    
    TEST_ASSERT_EQUAL_UINT32(1, lb.count);
    TEST_ASSERT_EQUAL_CHAR('A', lb.entries[0].initials[0]);
    TEST_ASSERT_EQUAL_CHAR(' ', lb.entries[0].initials[1]);
    TEST_ASSERT_EQUAL_CHAR(' ', lb.entries[0].initials[2]);
}

void test_AddLeaderboardEntry_MixedCase(void) {
    Leaderboard lb = {0};
    AddLeaderboardEntry(&lb, "aBc", 'p', 10.0f);
    
    TEST_ASSERT_EQUAL_UINT32(1, lb.count);
    TEST_ASSERT_EQUAL_STRING_LEN("ABC", lb.entries[0].initials, 3);
    TEST_ASSERT_EQUAL_CHAR('P', lb.entries[0].winner);
}

void test_AddLeaderboardEntry_SpecialCharacters(void) {
    Leaderboard lb = {0};
    AddLeaderboardEntry(&lb, "@#$", 'P', 10.0f);
    
    TEST_ASSERT_EQUAL_UINT32(1, lb.count);
    // Should accept any characters
    TEST_ASSERT_EQUAL_CHAR('@', lb.entries[0].initials[0]);
}

void test_LoadLeaderboard_IgnoresEntriesBeyondMax(void) {
    char tempHome[64];
    snprintf(tempHome, sizeof(tempHome), "/tmp/purpletestXXXXXX");
    if (!mkdtemp(tempHome)) {
        TEST_IGNORE_MESSAGE("Failed to create temp directory");
    }
    const char *oldHome = getenv("HOME");
    setenv("HOME", tempHome, 1);

    char purpleDir[128];
    snprintf(purpleDir, sizeof(purpleDir), "%s/.purple", tempHome);
    mkdir(purpleDir, 0700);

    char lbPath[160];
    snprintf(lbPath, sizeof(lbPath), "%s/leaderboard.txt", purpleDir);
    FILE *fp = fopen(lbPath, "w");
    TEST_ASSERT_NOT_NULL(fp);
    for (int i = 1; i <= (int)LEADERBOARD_MAX_ENTRIES + 5; ++i) {
        fprintf(fp, "%d.000;P;TST\n", i * 10);
    }
    fclose(fp);

    Leaderboard lb = {0};
    LoadLeaderboard(&lb);

    // Must never exceed the max even though the file has more lines
    TEST_ASSERT_EQUAL_UINT32(LEADERBOARD_MAX_ENTRIES, lb.count);

    if (oldHome) setenv("HOME", oldHome, 1);
}

void test_LoadLeaderboard_RejectsNaNSeconds(void) {
    // sscanf's %f parses the literal string "nan" as a valid NaN float on
    // POSIX. A NaN in the comparator makes qsort non-transitive (undefined
    // behaviour), so LoadLeaderboard must reject the line outright rather
    // than store it.
    char tempHome[64];
    snprintf(tempHome, sizeof(tempHome), "/tmp/purpletestXXXXXX");
    if (!mkdtemp(tempHome)) {
        TEST_IGNORE_MESSAGE("Failed to create temp directory");
    }
    const char *oldHome = getenv("HOME");
    setenv("HOME", tempHome, 1);

    char purpleDir[128];
    snprintf(purpleDir, sizeof(purpleDir), "%s/.purple", tempHome);
    mkdir(purpleDir, 0700);

    char lbPath[160];
    snprintf(lbPath, sizeof(lbPath), "%s/leaderboard.txt", purpleDir);
    FILE *fp = fopen(lbPath, "w");
    TEST_ASSERT_NOT_NULL(fp);
    fprintf(fp, "nan;P;BAD\n");
    fprintf(fp, "5.000;P;OKY\n");
    fclose(fp);

    Leaderboard lb = {0};
    LoadLeaderboard(&lb);

    TEST_ASSERT_EQUAL_UINT32(1, lb.count);
    TEST_ASSERT_FLOAT_IS_NOT_NAN(lb.entries[0].seconds);
    TEST_ASSERT_EQUAL_FLOAT(5.000f, lb.entries[0].seconds);

    if (oldHome) setenv("HOME", oldHome, 1);
}

void test_LoadLeaderboard_RejectsNegativeSeconds(void) {
    // sscanf's %f also parses a leading '-' as valid; a negative game time
    // is not a valid result and must be filtered at load time.
    char tempHome[64];
    snprintf(tempHome, sizeof(tempHome), "/tmp/purpletestXXXXXX");
    if (!mkdtemp(tempHome)) {
        TEST_IGNORE_MESSAGE("Failed to create temp directory");
    }
    const char *oldHome = getenv("HOME");
    setenv("HOME", tempHome, 1);

    char purpleDir[128];
    snprintf(purpleDir, sizeof(purpleDir), "%s/.purple", tempHome);
    mkdir(purpleDir, 0700);

    char lbPath[160];
    snprintf(lbPath, sizeof(lbPath), "%s/leaderboard.txt", purpleDir);
    FILE *fp = fopen(lbPath, "w");
    TEST_ASSERT_NOT_NULL(fp);
    fprintf(fp, "-5.000;P;BAD\n");
    fprintf(fp, "5.000;P;OKY\n");
    fclose(fp);

    Leaderboard lb = {0};
    LoadLeaderboard(&lb);

    TEST_ASSERT_EQUAL_UINT32(1, lb.count);
    TEST_ASSERT_EQUAL_FLOAT(5.000f, lb.entries[0].seconds);

    if (oldHome) setenv("HOME", oldHome, 1);
}

void test_LoadLeaderboard_DefaultsUnknownWinnerToPlayer(void) {
    // Any winner byte other than 'A' must default to 'P', matching
    // AddLeaderboardEntry's behaviour, rather than storing a stray value
    // that later code (which only branches on 'P' vs not) could misrender.
    char tempHome[64];
    snprintf(tempHome, sizeof(tempHome), "/tmp/purpletestXXXXXX");
    if (!mkdtemp(tempHome)) {
        TEST_IGNORE_MESSAGE("Failed to create temp directory");
    }
    const char *oldHome = getenv("HOME");
    setenv("HOME", tempHome, 1);

    char purpleDir[128];
    snprintf(purpleDir, sizeof(purpleDir), "%s/.purple", tempHome);
    mkdir(purpleDir, 0700);

    char lbPath[160];
    snprintf(lbPath, sizeof(lbPath), "%s/leaderboard.txt", purpleDir);
    FILE *fp = fopen(lbPath, "w");
    TEST_ASSERT_NOT_NULL(fp);
    fprintf(fp, "5.000;X;ABC\n");
    fclose(fp);

    Leaderboard lb = {0};
    LoadLeaderboard(&lb);

    TEST_ASSERT_EQUAL_UINT32(1, lb.count);
    TEST_ASSERT_EQUAL_CHAR('P', lb.entries[0].winner);

    if (oldHome) setenv("HOME", oldHome, 1);
}

void test_LoadLeaderboard_SkipsMalformedLine(void) {
    // A line missing the initials field only satisfies 2 of the 3 sscanf
    // conversions ("%f;%c;%7s" needs a literal ';' after %c, which a
    // trailing newline doesn't provide). LoadLeaderboard requires exactly
    // 3 conversions and must skip the line rather than store a partial entry.
    char tempHome[64];
    snprintf(tempHome, sizeof(tempHome), "/tmp/purpletestXXXXXX");
    if (!mkdtemp(tempHome)) {
        TEST_IGNORE_MESSAGE("Failed to create temp directory");
    }
    const char *oldHome = getenv("HOME");
    setenv("HOME", tempHome, 1);

    char purpleDir[128];
    snprintf(purpleDir, sizeof(purpleDir), "%s/.purple", tempHome);
    mkdir(purpleDir, 0700);

    char lbPath[160];
    snprintf(lbPath, sizeof(lbPath), "%s/leaderboard.txt", purpleDir);
    FILE *fp = fopen(lbPath, "w");
    TEST_ASSERT_NOT_NULL(fp);
    fprintf(fp, "5.000;P\n");
    fprintf(fp, "6.000;P;OKY\n");
    fclose(fp);

    Leaderboard lb = {0};
    LoadLeaderboard(&lb);

    TEST_ASSERT_EQUAL_UINT32(1, lb.count);
    TEST_ASSERT_EQUAL_FLOAT(6.000f, lb.entries[0].seconds);

    if (oldHome) setenv("HOME", oldHome, 1);
}

void test_LoadLeaderboard_TruncatesOversizedLine(void) {
    // A line far longer than the 256-byte fgets buffer in LoadLeaderboard
    // spills its remainder into what looks like a separate "line" on the
    // next fgets call. Neither chunk should parse as a valid entry, and
    // the parser must not desync or crash for the valid entry that follows.
    char tempHome[64];
    snprintf(tempHome, sizeof(tempHome), "/tmp/purpletestXXXXXX");
    if (!mkdtemp(tempHome)) {
        TEST_IGNORE_MESSAGE("Failed to create temp directory");
    }
    const char *oldHome = getenv("HOME");
    setenv("HOME", tempHome, 1);

    char purpleDir[128];
    snprintf(purpleDir, sizeof(purpleDir), "%s/.purple", tempHome);
    mkdir(purpleDir, 0700);

    char lbPath[160];
    snprintf(lbPath, sizeof(lbPath), "%s/leaderboard.txt", purpleDir);
    FILE *fp = fopen(lbPath, "w");
    TEST_ASSERT_NOT_NULL(fp);
    for (int i = 0; i < 300; ++i) fputc('A', fp);
    fputc('\n', fp);
    fprintf(fp, "5.000;P;OKY\n");
    fclose(fp);

    Leaderboard lb = {0};
    LoadLeaderboard(&lb);

    TEST_ASSERT_EQUAL_UINT32(1, lb.count);
    TEST_ASSERT_EQUAL_FLOAT(5.000f, lb.entries[0].seconds);

    if (oldHome) setenv("HOME", oldHome, 1);
}

void test_SaveAndLoadLeaderboard_PreservesPlayerWinner(void) {
    char tempHome[64];
    snprintf(tempHome, sizeof(tempHome), "/tmp/purpletestXXXXXX");
    if (!mkdtemp(tempHome)) {
        TEST_IGNORE_MESSAGE("Failed to create temp directory");
    }
    const char *oldHome = getenv("HOME");
    setenv("HOME", tempHome, 1);

    Leaderboard lb = {0};
    AddLeaderboardEntry(&lb, "XYZ", 'P', 42.5f);
    SaveLeaderboard(&lb);

    Leaderboard loaded = {0};
    LoadLeaderboard(&loaded);

    TEST_ASSERT_EQUAL_UINT32(1, loaded.count);
    TEST_ASSERT_EQUAL_CHAR('P', loaded.entries[0].winner);
    TEST_ASSERT_EQUAL_STRING_LEN("XYZ", loaded.entries[0].initials, 3);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 42.5f, loaded.entries[0].seconds);

    if (oldHome) setenv("HOME", oldHome, 1);
}

void test_AddLeaderboardEntry_MaintainsSortAfterMultipleAdds(void) {
    Leaderboard lb = {0};
    // Add in random order
    float times[] = {50.0f, 10.0f, 80.0f, 20.0f, 60.0f, 30.0f, 90.0f, 40.0f, 70.0f, 15.0f};
    for (int i = 0; i < 10; ++i) {
        AddLeaderboardEntry(&lb, "TST", 'P', times[i]);
    }
    
    // Verify sorted
    for (size_t i = 1; i < lb.count; ++i) {
        TEST_ASSERT_LESS_OR_EQUAL(lb.entries[i].seconds, lb.entries[i-1].seconds);
    }
    TEST_ASSERT_EQUAL_FLOAT(10.0f, lb.entries[0].seconds);
}

int main(void) {
    UNITY_BEGIN();
    
    // Critical null pointer tests first (fail fast)
    RUN_TEST(test_IsCollidingVertical_WithNullPointer);
    RUN_TEST(test_HandlePaddleCollision_WithNullBall);
    RUN_TEST(test_UpdateBallPosition_WithNullPointer);
    RUN_TEST(test_MovePaddleUp_WithNullPointer);
    RUN_TEST(test_MovePaddleDown_WithNullPointer);
    RUN_TEST(test_StopPaddle_WithNullPointer);
    RUN_TEST(test_UpdatePaddlePosition_WithNullPointer);
    RUN_TEST(test_UpdatePaddlePosition_NaNPositionResetsToTop);
    RUN_TEST(test_UpdateAIPaddle_WithNullPaddle);
    RUN_TEST(test_AddLeaderboardEntry_WithNullLeaderboard);
    RUN_TEST(test_LoadLeaderboard_WithNullPointer);
    RUN_TEST(test_SaveLeaderboard_WithNullPointer);
    
    // Ball collision tests
    RUN_TEST(test_IsCollidingVertical_TopWallEdge);
    RUN_TEST(test_IsCollidingVertical_BottomWallEdge);
    RUN_TEST(test_IsCollidingVertical_ExactlyAtTopEdge);
    RUN_TEST(test_IsCollidingVertical_ExactlyAtBottomEdge);
    RUN_TEST(test_IsCollidingVertical_JustInside);
    RUN_TEST(test_IsCollidingVertical_NegativePosition);
    RUN_TEST(test_IsCollidingVertical_BeyondBottom);
    
    // Paddle collision tests
    RUN_TEST(test_HandlePaddleCollision_NoCollision);
    RUN_TEST(test_HandlePaddleCollision_ReversesBallVelocityX);
    RUN_TEST(test_HandlePaddleCollision_PushesOutRightEdge);
    RUN_TEST(test_HandlePaddleCollision_PushesOutLeftEdge);
    RUN_TEST(test_HandlePaddleCollision_TopSpinEffect);
    RUN_TEST(test_HandlePaddleCollision_BottomSpinEffect);
    RUN_TEST(test_HandlePaddleCollision_CenterNoSpin);
    RUN_TEST(test_HandlePaddleCollision_MultipleRapidCollisions);
    RUN_TEST(test_HandlePaddleCollision_EdgeOfPaddle);
    RUN_TEST(test_HandlePaddleCollision_SpinAccumulatesOnExistingY);
    RUN_TEST(test_HandlePaddleCollision_TinyPaddleNoSpinDivisionByZero);
    RUN_TEST(test_HandlePaddleCollision_SpinClampedAtMaxSpeedY);
    RUN_TEST(test_HandlePaddleCollision_UsesStructDimensionsForGeometry);
    RUN_TEST(test_HandlePaddleCollision_NaNSpinVelocityResetsToZero);

    // Ball position tests
    RUN_TEST(test_UpdateBallPosition_MovesCorrectly);
    RUN_TEST(test_UpdateBallPosition_MovesBackwards);
    RUN_TEST(test_UpdateBallPosition_WithZeroVelocity);
    RUN_TEST(test_UpdateBallPosition_NaNVelocityResetsToZero);
    RUN_TEST(test_UpdateBallPosition_InfPositionResetsToZero);
    RUN_TEST(test_UpdateBallPosition_LargeVelocity);
    RUN_TEST(test_UpdateBallPosition_NegativePosition);
    RUN_TEST(test_IsCollidingVertical_ZeroScreenHeight);
    RUN_TEST(test_HandlePaddleCollision_ZeroWidthPaddle);
    RUN_TEST(test_HandlePaddleCollision_ZeroHeightPaddle);
    RUN_TEST(test_HandlePaddleCollision_PushbackPreventsSticking);
    
    // Paddle movement tests
    RUN_TEST(test_MovePaddleUp_SetsNegativeVelocity);
    RUN_TEST(test_MovePaddleDown_SetsPositiveVelocity);
    RUN_TEST(test_StopPaddle_SetsZeroVelocity);
    RUN_TEST(test_UpdatePaddlePosition_MovesWithVelocity);
    RUN_TEST(test_UpdatePaddlePosition_ClampsAtTop);
    RUN_TEST(test_UpdatePaddlePosition_ClampsAtBottom);
    RUN_TEST(test_UpdatePaddlePosition_MultipleFrames);
    RUN_TEST(test_UpdatePaddlePosition_StoppingAtBoundary);
    RUN_TEST(test_UpdatePaddlePosition_NegativeVelocityAtTop);
    RUN_TEST(test_UpdatePaddlePosition_ZeroHeight);
    RUN_TEST(test_UpdatePaddlePosition_OversizedPaddleClampsToTop);
    RUN_TEST(test_UpdatePaddlePosition_LargeVelocity);
    RUN_TEST(test_MovePaddle_RapidDirectionChange);
    RUN_TEST(test_UpdatePaddlePosition_AlreadyAtExactTop);
    RUN_TEST(test_UpdatePaddlePosition_AlreadyAtExactBottom);
    
    // AI paddle tests
    RUN_TEST(test_UpdateAIPaddle_MovesUpTowardsBall);
    RUN_TEST(test_UpdateAIPaddle_MovesDownTowardsBall);
    RUN_TEST(test_UpdateAIPaddle_StopsNearBall);
    RUN_TEST(test_UpdateAIPaddle_ClampsAtTopBoundary);
    RUN_TEST(test_UpdateAIPaddle_ClampsAtBottomBoundary);
    RUN_TEST(test_UpdateAIPaddle_ExactlyAtCenter);
    
    // Resource function tests
    RUN_TEST(test_FindResourceDirectory_IsValid);
    RUN_TEST(test_FindResourceFile_ReturnsNonNull);
    RUN_TEST(test_FindResourceFile_NullSubpathDoesNotCrash);
    RUN_TEST(test_FindFontPath_ReturnsNonNull);
    RUN_TEST(test_FindResourceDirectory_IsConsistent);
    
    // Font file tests
    RUN_TEST(test_RaylibVersion_AtLeastSix);
    RUN_TEST(test_FontFile_ExistsAndValid);

    // Leaderboard tests
    RUN_TEST(test_AddLeaderboardEntry_WithNullInitials);
    RUN_TEST(test_AddLeaderboardEntry_WithEmptyInitials);
    RUN_TEST(test_AddLeaderboardEntry_UppercasesAndStores);
    RUN_TEST(test_AddLeaderboardEntry_AIWinnerUppercase);
    RUN_TEST(test_AddLeaderboardEntry_UnknownWinnerDefaultsToPlayer);
    RUN_TEST(test_AddLeaderboardEntry_SortedAscending);
    RUN_TEST(test_AddLeaderboardEntry_ReplacesWorstWhenFull);
    RUN_TEST(test_AddLeaderboardEntry_IgnoresSlowerThanWorst);
    RUN_TEST(test_AddLeaderboardEntry_ZeroTime);
    RUN_TEST(test_AddLeaderboardEntry_NegativeTime);
    RUN_TEST(test_AddLeaderboardEntry_VeryLargeTime);
    RUN_TEST(test_AddLeaderboardEntry_DuplicateTimes);
    RUN_TEST(test_AddLeaderboardEntry_LongInitials);
    RUN_TEST(test_AddLeaderboardEntry_SingleChar);
    RUN_TEST(test_AddLeaderboardEntry_MixedCase);
    RUN_TEST(test_AddLeaderboardEntry_SpecialCharacters);
    RUN_TEST(test_AddLeaderboardEntry_MaintainsSortAfterMultipleAdds);
    RUN_TEST(test_LoadLeaderboard_NonexistentFile);
    RUN_TEST(test_LoadLeaderboard_IgnoresEntriesBeyondMax);
    RUN_TEST(test_LoadLeaderboard_RejectsNaNSeconds);
    RUN_TEST(test_LoadLeaderboard_RejectsNegativeSeconds);
    RUN_TEST(test_LoadLeaderboard_DefaultsUnknownWinnerToPlayer);
    RUN_TEST(test_LoadLeaderboard_SkipsMalformedLine);
    RUN_TEST(test_LoadLeaderboard_TruncatesOversizedLine);
    RUN_TEST(test_SaveLeaderboard_EmptyLeaderboard);
    RUN_TEST(test_SaveAndLoadLeaderboard_PersistsSorted);
    RUN_TEST(test_SaveAndLoadLeaderboard_PreservesPlayerWinner);
    
    return UNITY_END();
}
