/* =========================================================================
    Purple
    https://github.com/octopusnz/purple
    Copyright (c) 2026 Jacob Doherty
    SPDX-License-Identifier: MIT
    File: main.c
    Description: Main game loop and rendering
========================================================================= */

#include <raylib/raylib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ball.h"
#include "paddle.h"
#include "resource.h"
#include "leaderboard.h"

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 600
#define PADDLE_WIDTH 15.0f
#define PADDLE_HEIGHT 100.0f
#define POINTS_TO_WIN 5
#define PADDLE_OFFSET 20.0f
#define BALL_RADIUS 8.0f
#define BALL_INITIAL_SPEED_X 6.0f
#define BALL_INITIAL_SPEED_Y 3.0f
#define SPEED_INCREMENT_PER_POINT 0.12f
#define CENTER_LINE_SEGMENT 28
#define CENTER_LINE_DASH 16
#define TITLE_FONT_SIZE 48
#define SCORE_FONT_SIZE 28
#define MESSAGE_FONT_SIZE 24
#define GAME_OVER_FONT_SIZE 40
#define SCORE_DISPLAY_FONT_SIZE 64

typedef enum {
    START_SCREEN,
    PLAYING,
    NAME_ENTRY
} GameState;

static float CalculateSpeedMultiplier(int totalScore)
{
    return 1.0f + (float)totalScore * SPEED_INCREMENT_PER_POINT;
}

static void ResetBall(Ball *ball, int screenWidth, int screenHeight, float speedMultiplier)
{
    ball->position.x = (float)screenWidth / 2.0f;
    ball->position.y = (float)screenHeight / 2.0f;
    
    // Randomize ball direction (50% chance to go left or right)
    int direction = (rand() % 2 == 0) ? 1 : -1;
    ball->velocity.x = BALL_INITIAL_SPEED_X * speedMultiplier * (float)direction;
    ball->velocity.y = BALL_INITIAL_SPEED_Y * speedMultiplier * ((rand() % 2 == 0) ? 1.0f : -1.0f);
}

static void DrawPaddle(const Paddle *paddle, Color colour)
{
    Rectangle rec = { paddle->position.x, paddle->position.y, paddle->width, paddle->height };
    DrawRectangleRounded(rec, 0.4f, 8, colour);
}

static void DrawCenteredText(Font font, const char *text, int y, int fontSize, Color colour)
{
    Vector2 textSize = MeasureTextEx(font, text, (float)fontSize, 1);
    float x = (SCREEN_WIDTH - textSize.x) / 2.0f;
    DrawTextEx(font, text, (Vector2){x, (float)y}, (float)fontSize, 1, colour);
}

int main(void)
{
    // Initialization
    srand((unsigned int)time(NULL));  // Seed random number generator
    SetTraceLogLevel(LOG_WARNING);    // Suppress INFO messages from raylib init
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Purple - Pong");
    SetTargetFPS(60);

    // Load custom font from multiple possible locations
    Font orbitronFont = LoadFontEx(FindFontPath(), 128, 0, 0);
    SetTextureFilter(orbitronFont.texture, TEXTURE_FILTER_BILINEAR);

    // Initialize ball
    Ball ball = {
        .position = { (float)SCREEN_WIDTH / 2.0f, (float)SCREEN_HEIGHT / 2.0f },
        .velocity = { 0.0f, 0.0f },
        .radius = BALL_RADIUS
    };

    // Initialize player paddle (left side)
    Paddle player = {
        .position = { PADDLE_OFFSET, (float)(SCREEN_HEIGHT - PADDLE_HEIGHT) / 2.0f },
        .width = PADDLE_WIDTH,
        .height = PADDLE_HEIGHT,
        .velocity = 0.0f,
        .score = 0
    };

    // Initialize AI paddle (right side)
    Paddle ai = {
        .position = { (float)SCREEN_WIDTH - PADDLE_WIDTH - PADDLE_OFFSET,
                      (float)(SCREEN_HEIGHT - PADDLE_HEIGHT) / 2.0f },
        .width = PADDLE_WIDTH,
        .height = PADDLE_HEIGHT,
        .velocity = 0.0f,
        .score = 0
    };

    GameState gameState = START_SCREEN;
    float ballSpeedMultiplier = 1.0f;
    double gameStartTime = 0.0;
    float lastGameSeconds = 0.0f;

    Leaderboard leaderboard;
    LoadLeaderboard(&leaderboard);

    char initials[4] = {' ', ' ', ' ', '\0'};
    int initialsCount = 0;

    // Score display strings; rebuilt only when a score changes, not every frame
    char playerScoreText[20];
    char aiScoreText[20];
    snprintf(playerScoreText, sizeof(playerScoreText), "%d", player.score);
    snprintf(aiScoreText, sizeof(aiScoreText), "%d", ai.score);
    // Cached text sizes; updated alongside the score strings
    Vector2 playerScoreSize = MeasureTextEx(orbitronFont, playerScoreText,
                                            (float)SCORE_DISPLAY_FONT_SIZE, 1);
    Vector2 aiScoreSize = MeasureTextEx(orbitronFont, aiScoreText,
                                        (float)SCORE_DISPLAY_FONT_SIZE, 1);
    
    // Set initial ball velocity using multiplier
    ResetBall(&ball, SCREEN_WIDTH, SCREEN_HEIGHT, ballSpeedMultiplier);

    // Main game loop
    while (!WindowShouldClose())
    {
        // Update
        if (gameState == START_SCREEN)
        {
            if (IsKeyPressed(KEY_SPACE)) {
                // Reset scores and positions for new game
                player.score = 0;
                ai.score = 0;
                ballSpeedMultiplier = 1.0f;
                ResetBall(&ball, SCREEN_WIDTH, SCREEN_HEIGHT, ballSpeedMultiplier);
                player.position.y = (float)(SCREEN_HEIGHT - PADDLE_HEIGHT) / 2.0f;
                ai.position.y = (float)(SCREEN_HEIGHT - PADDLE_HEIGHT) / 2.0f;
                gameStartTime = GetTime();
                gameState = PLAYING;
            }
        }
        else if (gameState == PLAYING)
        {
            // Player input
            if (IsKeyDown(KEY_UP)) {
                MovePaddleUp(&player);
            } else if (IsKeyDown(KEY_DOWN)) {
                MovePaddleDown(&player);
            } else {
                StopPaddle(&player);
            }

            // Update positions
            UpdatePaddlePosition(&player, SCREEN_HEIGHT);
            UpdateAIPaddle(&ai, ball.position, ball.radius, SCREEN_HEIGHT);
            UpdateBallPosition(&ball);

            // Handle paddle collisions
            HandlePaddleCollision(&ball, player.position, player.width, player.height);
            HandlePaddleCollision(&ball, ai.position, ai.width, ai.height);

            // Handle top/bottom wall collisions
            if (IsCollidingVertical(&ball, SCREEN_HEIGHT)) {
                ball.velocity.y *= -1.0f;
                // Correct position to prevent re-collision on next frame
                if (ball.position.y - ball.radius < 0.0f) {
                    ball.position.y = ball.radius;
                } else if (ball.position.y + ball.radius > (float)SCREEN_HEIGHT) {
                    ball.position.y = (float)SCREEN_HEIGHT - ball.radius;
                }
            }

            // Check for scoring (ball goes off left or right)
            if (ball.position.x < 0.0f) {
                // AI scores
                ai.score++;
                snprintf(aiScoreText, sizeof(aiScoreText), "%d", ai.score);
                aiScoreSize = MeasureTextEx(orbitronFont, aiScoreText,
                                            (float)SCORE_DISPLAY_FONT_SIZE, 1);
                ballSpeedMultiplier = CalculateSpeedMultiplier(ai.score + player.score);
                if (ai.score >= POINTS_TO_WIN) {
                    lastGameSeconds = (float)(GetTime() - gameStartTime);
                    // Save AI win automatically
                    AddLeaderboardEntry(&leaderboard, "AI", 'A', lastGameSeconds);
                    SaveLeaderboard(&leaderboard);
                    gameState = START_SCREEN;
                } else {
                    ResetBall(&ball, SCREEN_WIDTH, SCREEN_HEIGHT, ballSpeedMultiplier);
                }
            } else if (ball.position.x > SCREEN_WIDTH) {
                // Player scores
                player.score++;
                snprintf(playerScoreText, sizeof(playerScoreText), "%d", player.score);
                playerScoreSize = MeasureTextEx(orbitronFont, playerScoreText,
                                                (float)SCORE_DISPLAY_FONT_SIZE, 1);
                ballSpeedMultiplier = CalculateSpeedMultiplier(ai.score + player.score);
                if (player.score >= POINTS_TO_WIN) {
                    lastGameSeconds = (float)(GetTime() - gameStartTime);
                    // Move to initials entry state
                    initials[0] = initials[1] = initials[2] = ' ';
                    initials[3] = '\0';
                    initialsCount = 0;
                    gameState = NAME_ENTRY;
                } else {
                    ResetBall(&ball, SCREEN_WIDTH, SCREEN_HEIGHT, ballSpeedMultiplier);
                }
            }
        }
        else if (gameState == NAME_ENTRY)
        {
            // Handle initials input (A-Z), backspace, and enter to save
            int key = GetCharPressed();
            while (key > 0) {
                if ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z')) {
                    if (initialsCount < 3) {
                        char c = (char)key;
                        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
                        initials[initialsCount++] = c;
                    }
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && initialsCount > 0) {
                initials[--initialsCount] = ' ';
            }
            if (IsKeyPressed(KEY_ENTER) && initialsCount > 0) {
                AddLeaderboardEntry(&leaderboard, initials, 'P', lastGameSeconds);
                SaveLeaderboard(&leaderboard);
                gameState = START_SCREEN;
            }
        }

        // Draw
        BeginDrawing();
        ClearBackground((Color){10, 10, 25, 255});

        // Top and bottom wall lines
        DrawLineV((Vector2){0.0f, 0.0f},
                  (Vector2){(float)SCREEN_WIDTH, 0.0f}, (Color){255, 255, 255, 35});
        DrawLineV((Vector2){0.0f, (float)(SCREEN_HEIGHT - 1)},
                  (Vector2){(float)SCREEN_WIDTH, (float)(SCREEN_HEIGHT - 1)},
                  (Color){255, 255, 255, 35});

        // Draw center dashed line only during active gameplay
        if (gameState == PLAYING) {
            float cx = (float)(SCREEN_WIDTH / 2);
            float halfH = (float)SCREEN_HEIGHT / 2.0f;
            for (int i = 0; i < SCREEN_HEIGHT; i += CENTER_LINE_SEGMENT) {
                float dashCenterY = (float)i + CENTER_LINE_DASH / 2.0f;
                float dist = fabsf(dashCenterY - halfH);
                float t = 1.0f - dist / halfH;          // 1 at midfield, 0 at edges
                unsigned char alpha = (unsigned char)(30 + (int)(90.0f * t * t));
                Rectangle r = { cx - 3.0f, (float)i, 6.0f, (float)CENTER_LINE_DASH };
                DrawRectangleRounded(r, 0.8f, 4, (Color){255, 255, 255, alpha});
            }
        }

        // Draw title
        DrawCenteredText(orbitronFont, "PURPLE", 10, TITLE_FONT_SIZE, WHITE);

        if (gameState == START_SCREEN) {
            DrawCenteredText(orbitronFont, "FASTEST WINS", 90, SCORE_FONT_SIZE,
                             (Color){200, 200, 220, 255});
            int startY = 135;
            for (size_t i = 0; i < leaderboard.count; ++i) {
                const LeaderboardEntry *e = &leaderboard.entries[i];
                char line[128];
                snprintf(line, sizeof(line), "%2zu.  %6.3fs   %c   %s",
                         i + 1, (double)e->seconds, e->winner, e->initials);
                Color rowColor = (e->winner == 'P')
                    ? (Color){80, 160, 255, 255}
                    : (Color){255, 80, 80, 255};
                DrawCenteredText(orbitronFont, line, startY + (int)i * 32,
                                 MESSAGE_FONT_SIZE, rowColor);
            }
            DrawCenteredText(orbitronFont, "PRESS SPACE TO PLAY",
                             SCREEN_HEIGHT - 70, MESSAGE_FONT_SIZE,
                             (Color){180, 180, 200, 255});
        } else if (gameState == PLAYING) {
            DrawPaddle(&player, (Color){80, 160, 255, 255});
            DrawPaddle(&ai, (Color){255, 80, 80, 255});

            // Ball: outer glow, inner glow, solid core
            DrawCircleV(ball.position, ball.radius + 6.0f, (Color){150, 80, 255, 35});
            DrawCircleV(ball.position, ball.radius + 3.0f, (Color){200, 140, 255, 80});
            DrawCircleV(ball.position, ball.radius, WHITE);

            // Scores: large numbers centered in each half
            DrawTextEx(orbitronFont, playerScoreText,
                       (Vector2){SCREEN_WIDTH / 4.0f - playerScoreSize.x / 2.0f, 20.0f},
                       (float)SCORE_DISPLAY_FONT_SIZE, 1, (Color){80, 160, 255, 130});

            DrawTextEx(orbitronFont, aiScoreText,
                       (Vector2){3.0f * SCREEN_WIDTH / 4.0f - aiScoreSize.x / 2.0f, 20.0f},
                       (float)SCORE_DISPLAY_FONT_SIZE, 1, (Color){255, 80, 80, 130});
        } else if (gameState == NAME_ENTRY) {
            DrawCenteredText(orbitronFont, "YOU WIN!", 180,
                             GAME_OVER_FONT_SIZE, (Color){80, 255, 120, 255});

            // Build display with underscores for empty slots and blinking cursor
            char display[4];
            for (int i = 0; i < 3; i++) {
                if (i < initialsCount) {
                    display[i] = initials[i];
                } else if (i == initialsCount && (int)(GetTime() * 2) % 2 == 0) {
                    display[i] = '|';
                } else {
                    display[i] = '_';
                }
            }
            display[3] = '\0';
            char prompt[32];
            snprintf(prompt, sizeof(prompt), "INITIALS: %s", display);
            DrawCenteredText(orbitronFont, prompt, 260,
                             GAME_OVER_FONT_SIZE - 8, WHITE);
            DrawCenteredText(orbitronFont, "PRESS ENTER TO SAVE", 330,
                             MESSAGE_FONT_SIZE, (Color){140, 140, 160, 255});
        }

        // FPS counter (subtle, top-right)
        DrawTextEx(orbitronFont, TextFormat("%d FPS", GetFPS()),
                   (Vector2){(float)SCREEN_WIDTH - 90.0f, 12.0f}, 16.0f, 1,
                   (Color){255, 255, 255, 60});

        EndDrawing();
    }

    // De-Initialization
    UnloadFont(orbitronFont);
    CloseWindow();
    return 0;
}