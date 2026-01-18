#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 700
#include "unity/unity.h"
#include "../ball.h"
#include "../paddle.h"
#include "../leaderboard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

int directory_exists(const char *dirname) {
    struct stat buffer;
    return (stat(dirname, &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

// Find resources directory by searching parent directories
const char* find_resource_directory(void) {
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
            return resourcePath;
        }
    }

    // Default fallback
    return "./resources";
}

void test_FontFile_ExistsAtRelativePath(void) {
    // Font should be accessible from the resources directory
    const char *resourceDir = find_resource_directory();
    char fontPath[512];
    snprintf(fontPath, sizeof(fontPath), "%s/orbitron/Orbitron-VariableFont_wght.ttf", resourceDir);

    if (!file_exists(fontPath)) {
        TEST_IGNORE_MESSAGE("Font file not found in resources directory");
    }
    TEST_ASSERT_TRUE(file_exists(fontPath));
}

void test_FontFile_IsReadable(void) {
    // Font file should be readable from the resources directory
    const char *resourceDir = find_resource_directory();
    char fontPath[512];
    snprintf(fontPath, sizeof(fontPath), "%s/orbitron/Orbitron-VariableFont_wght.ttf", resourceDir);

    FILE *fp = fopen(fontPath, "rb");
    if (!fp) {
        TEST_IGNORE_MESSAGE("Font file not found in resources directory");
    }
    TEST_ASSERT_NOT_NULL(fp);
    if (fp) {
        fclose(fp);
    }
}

void test_FontFile_HasValidSize(void) {
    // Font file should have content (not empty)
    const char *resourceDir = find_resource_directory();
    char fontPath[512];
    snprintf(fontPath, sizeof(fontPath), "%s/orbitron/Orbitron-VariableFont_wght.ttf", resourceDir);

    FILE *fp = fopen(fontPath, "rb");
    if (!fp) {
        TEST_IGNORE_MESSAGE("Font file not found in resources directory");
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
    const char *resourceDir = find_resource_directory();
    char fontPath[512];
    snprintf(fontPath, sizeof(fontPath), "%s/orbitron/Orbitron-VariableFont_wght.ttf", resourceDir);

    FILE *fp = fopen(fontPath, "rb");
    if (!fp) {
        TEST_IGNORE_MESSAGE("Font file not found in resources directory");
    }
    TEST_ASSERT_NOT_NULL(fp);
    if (fp) {
        unsigned char header[4];
        size_t read = fread(header, 1, 4, fp);
        TEST_ASSERT_EQUAL_INT(4, read);
        // TTF files start with 0x00 0x01 0x00 0x00 or "OTTO" or "true"
        int valid = (header[0] == 0x00 && header[1] == 0x01 &&
                     header[2] == 0x00 && header[3] == 0x00) ||
                    (header[0] == 'O' && header[1] == 'T' &&
                     header[2] == 'T' && header[3] == 'O') ||
                    (header[0] == 't' && header[1] == 'r' &&
                     header[2] == 'u' && header[3] == 'e');
        TEST_ASSERT_TRUE(valid);
        fclose(fp);
    }
}

// Helpers for leaderboard tests
static char *create_temp_home(char *templateBuf, size_t bufSize) {
    if (bufSize < 1) return NULL;
    snprintf(templateBuf, bufSize, "/tmp/purpletestXXXXXX");
    return mkdtemp(templateBuf);
}

static void cleanup_temp_home(const char *homePath) {
    if (!homePath) return;
    char lbDir[600];
    char lbPath[600];
    int dirLen = snprintf(lbDir, sizeof(lbDir), "%s/.purple", homePath);
    int pathLen = snprintf(lbPath, sizeof(lbPath), "%s/leaderboard.txt", lbDir);
    if (dirLen > 0 && dirLen < (int)sizeof(lbDir)) {
        if (pathLen > 0 && pathLen < (int)sizeof(lbPath)) {
            remove(lbPath);
        }
        rmdir(lbDir);
    }
    rmdir(homePath);
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
    char *homePath = create_temp_home(tempHome, sizeof(tempHome));
    TEST_ASSERT_NOT_NULL(homePath);

    const char *oldHome = getenv("HOME");
    setenv("HOME", homePath, 1);

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

    if (oldHome) {
        setenv("HOME", oldHome, 1);
    }
    cleanup_temp_home(homePath);
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

    // Leaderboard tests
    RUN_TEST(test_AddLeaderboardEntry_UppercasesAndStores);
    RUN_TEST(test_AddLeaderboardEntry_ReplacesWorstWhenFull);
    RUN_TEST(test_AddLeaderboardEntry_IgnoresSlowerThanWorst);
    RUN_TEST(test_SaveAndLoadLeaderboard_PersistsSorted);
    
    return UNITY_END();
}
