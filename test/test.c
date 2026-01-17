#include "unity/unity.h"
#include "../ball.h"
#include "../paddle.h"
#include <stdio.h>
#include <sys/stat.h>

void setUp(void) {
    // Set up before each test
}

void tearDown(void) {
    // Clean up after each test
}

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

// Paddle tests
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

// Font file tests
int file_exists(const char *filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

void test_FontFile_ExistsAtRelativePath(void) {
    // Font should be accessible from ../resources/ when running from build directory
    // Skip test if file doesn't exist (might be running from different location)
    int exists = file_exists("../resources/orbitron/Orbitron-VariableFont_wght.ttf");
    if (!exists) {
        TEST_IGNORE_MESSAGE("Font file not found at expected path");
    }
    TEST_ASSERT_TRUE(exists);
}

void test_FontFile_IsReadable(void) {
    // Font file should be readable
    FILE *fp = fopen("../resources/orbitron/Orbitron-VariableFont_wght.ttf", "rb");
    if (!fp) {
        TEST_IGNORE_MESSAGE("Font file not found");
    }
    TEST_ASSERT_NOT_NULL(fp);
    if (fp) {
        fclose(fp);
    }
}

void test_FontFile_HasValidSize(void) {
    // Font file should have content (not empty)
    FILE *fp = fopen("../resources/orbitron/Orbitron-VariableFont_wght.ttf", "rb");
    if (!fp) {
        TEST_IGNORE_MESSAGE("Font file not found");
    }
    TEST_ASSERT_NOT_NULL(fp);
    if (fp) {
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        TEST_ASSERT_GREATER_THAN(0, size);  // File should not be empty
        fclose(fp);
    }
}

void test_FontFile_IsTTFFormat(void) {
    // TTF files start with specific magic bytes
    FILE *fp = fopen("../resources/orbitron/Orbitron-VariableFont_wght.ttf", "rb");
    if (!fp) {
        TEST_IGNORE_MESSAGE("Font file not found");
    }
    TEST_ASSERT_NOT_NULL(fp);
    if (fp) {
        unsigned char header[4];
        size_t read = fread(header, 1, 4, fp);
        TEST_ASSERT_EQUAL_INT(4, read);
        // TTF files start with either 0x00 0x01 0x00 0x00 or "OTTO" or "true"
        int valid = (header[0] == 0x00 && header[1] == 0x01 && header[2] == 0x00 && header[3] == 0x00) ||
                    (header[0] == 'O' && header[1] == 'T' && header[2] == 'T' && header[3] == 'O') ||
                    (header[0] == 't' && header[1] == 'r' && header[2] == 'u' && header[3] == 'e');
        TEST_ASSERT_TRUE(valid);
        fclose(fp);
    }
}

int main(void) {
    UNITY_BEGIN();
    
    // Ball position tests
    RUN_TEST(test_UpdateBallPosition_MovesCorrectly);
    RUN_TEST(test_UpdateBallPosition_WithNullPointer);
    RUN_TEST(test_UpdateBallPosition_MovesBackwards);
    RUN_TEST(test_UpdateBallPosition_WithZeroVelocity);
    
    // Ball collision tests
    RUN_TEST(test_IsCollidingVertical_TopWallEdge);
    RUN_TEST(test_IsCollidingVertical_BottomWallEdge);
    RUN_TEST(test_IsCollidingVertical_JustInside);
    
    // Paddle movement tests
    RUN_TEST(test_MovePaddleUp_SetsNegativeVelocity);
    RUN_TEST(test_MovePaddleDown_SetsPositiveVelocity);
    RUN_TEST(test_StopPaddle_SetsZeroVelocity);
    RUN_TEST(test_UpdatePaddlePosition_MovesWithVelocity);
    RUN_TEST(test_UpdatePaddlePosition_ClampsAtTop);
    RUN_TEST(test_UpdatePaddlePosition_ClampsAtBottom);
    RUN_TEST(test_UpdatePaddlePosition_WithNullPointer);
    RUN_TEST(test_UpdatePaddlePosition_MultipleFrames);
    RUN_TEST(test_UpdatePaddlePosition_StoppingAtBoundary);
    RUN_TEST(test_UpdatePaddlePosition_NegativeVelocityAtTop);
    
    // Paddle collision tests (spin mechanics)
    RUN_TEST(test_HandlePaddleCollision_ReversesBallVelocityX);
    RUN_TEST(test_HandlePaddleCollision_TopSpinEffect);
    RUN_TEST(test_HandlePaddleCollision_BottomSpinEffect);
    RUN_TEST(test_HandlePaddleCollision_CenterNoSpin);
    
    // AI paddle tests
    RUN_TEST(test_UpdateAIPaddle_MovesUpTowardsBall);
    RUN_TEST(test_UpdateAIPaddle_MovesDownTowardsBall);
    RUN_TEST(test_UpdateAIPaddle_StopsNearBall);
    RUN_TEST(test_UpdateAIPaddle_ClampsAtTopBoundary);
    RUN_TEST(test_UpdateAIPaddle_ClampsAtBottomBoundary);
    RUN_TEST(test_UpdateAIPaddle_WithNullPaddle);
    
    // Font tests
    RUN_TEST(test_FontFile_ExistsAtRelativePath);
    RUN_TEST(test_FontFile_IsReadable);
    RUN_TEST(test_FontFile_HasValidSize);
    RUN_TEST(test_FontFile_IsTTFFormat);
    
    return UNITY_END();
}
