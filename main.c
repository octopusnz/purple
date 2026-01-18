#include <raylib/raylib.h>
#include <stdio.h>
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
#define BALL_INITIAL_SPEED_X 4.0f
#define BALL_INITIAL_SPEED_Y 2.0f
#define SPEED_INCREMENT_PER_POINT 0.02f
#define CENTER_LINE_SEGMENT 20
#define CENTER_LINE_GAP 10
#define TITLE_FONT_SIZE 48
#define SCORE_FONT_SIZE 28
#define MESSAGE_FONT_SIZE 24
#define GAME_OVER_FONT_SIZE 40

typedef enum {
    START_SCREEN,
    PLAYING,
    PLAYER_WINS,
    AI_WINS,
    NAME_ENTRY
} GameState;

static void ResetBall(Ball *ball, int screenWidth, int screenHeight, float speedMultiplier)
{
    ball->position.x = (float)screenWidth / 2.0f;
    ball->position.y = (float)screenHeight / 2.0f;
    ball->velocity.x = BALL_INITIAL_SPEED_X * speedMultiplier;
    ball->velocity.y = BALL_INITIAL_SPEED_Y * speedMultiplier;
}

static void DrawPaddle(Paddle *paddle, Color colour)
{
    DrawRectangleV(paddle->position, (Vector2){PADDLE_WIDTH, PADDLE_HEIGHT}, colour);
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
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Purple - Pong");
    SetTargetFPS(60);

    // Load custom font from multiple possible locations
    Font orbitronFont = LoadFontEx(FindFontPath(), 32, 0, 0);

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
            HandlePaddleCollision(&ball, player.position, PADDLE_WIDTH, PADDLE_HEIGHT);
            HandlePaddleCollision(&ball, ai.position, PADDLE_WIDTH, PADDLE_HEIGHT);

            // Handle top/bottom wall collisions
            if (IsCollidingVertical(&ball, SCREEN_HEIGHT)) {
                ball.velocity.y *= -1.0f;
            }

            // Check for scoring (ball goes off left or right)
            if (ball.position.x < 0.0f) {
                // AI scores
                ai.score++;
                ballSpeedMultiplier = 1.0f + (float)(ai.score + player.score) * SPEED_INCREMENT_PER_POINT;
                if (ai.score >= POINTS_TO_WIN) {
                    lastGameSeconds = (float)(GetTime() - gameStartTime);
                    // Save AI win automatically
                    AddLeaderboardEntry(&leaderboard, "AI", 'A', lastGameSeconds);
                    SaveLeaderboard(&leaderboard);
                    // Go back to start screen
                    gameState = START_SCREEN;
                } else {
                    ResetBall(&ball, SCREEN_WIDTH, SCREEN_HEIGHT, ballSpeedMultiplier);
                }
            } else if (ball.position.x > SCREEN_WIDTH) {
                // Player scores
                player.score++;
                ballSpeedMultiplier = 1.0f + (float)(ai.score + player.score)
                                      * SPEED_INCREMENT_PER_POINT;
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
                // Return to start screen
                gameState = START_SCREEN;
            }
        }

        // Legacy game over restart via SPACE now handled in START_SCREEN

        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw center line
        for (int i = 0; i < SCREEN_HEIGHT; i += CENTER_LINE_SEGMENT) {
            DrawLineV((Vector2){ SCREEN_WIDTH / 2.0f, (float)i }, 
                      (Vector2){ SCREEN_WIDTH / 2.0f, (float)(i + CENTER_LINE_GAP) }, LIGHTGRAY);
        }

        // Draw title
        DrawCenteredText(orbitronFont, "PONG", 10, TITLE_FONT_SIZE, DARKGRAY);

        if (gameState == START_SCREEN) {
            // Show leaderboard
            DrawCenteredText(orbitronFont, "Fastest Wins", 80, SCORE_FONT_SIZE, DARKGRAY);
            int startY = 120;
            for (size_t i = 0; i < leaderboard.count; ++i) {
                const LeaderboardEntry *e = &leaderboard.entries[i];
                char line[128];
                snprintf(line, sizeof(line), "%2zu. %6.3fs  %c  %s",
                         i + 1, (double)e->seconds, e->winner, e->initials);
                DrawCenteredText(orbitronFont, line, startY + (int)i * 30,
                                
                                 MESSAGE_FONT_SIZE, BLACK);
            }
            DrawCenteredText(orbitronFont, "Press SPACE to play",
                             SCREEN_HEIGHT - 80, MESSAGE_FONT_SIZE, DARKGRAY);
        } else {
            // Draw paddles and ball during gameplay or name entry
            DrawPaddle(&player, BLUE);
            DrawPaddle(&ai, RED);
            DrawCircleV(ball.position, ball.radius, PURPLE);

            // Draw scores
            char playerScoreText[20], aiScoreText[20];
            snprintf(playerScoreText, sizeof(playerScoreText), "Player: %d", player.score);
            snprintf(aiScoreText, sizeof(aiScoreText), "AI: %d", ai.score);

            DrawTextEx(orbitronFont, playerScoreText, (Vector2){50, 80},
                       SCORE_FONT_SIZE, 1, BLUE);
            DrawTextEx(orbitronFont, aiScoreText,
                       (Vector2){SCREEN_WIDTH - 250, 80}, SCORE_FONT_SIZE, 1, RED);
        }

        // Draw FPS
        DrawFPS(10, 10);

        // Draw game state messages
        if (gameState == NAME_ENTRY) {
            DrawCenteredText(orbitronFont, "YOU WIN!", 220,
                             GAME_OVER_FONT_SIZE, GREEN);
            char prompt[64];
            snprintf(prompt, sizeof(prompt), "Enter Initials: %c%c%c",
                     initials[0], initials[1], initials[2]);
            DrawCenteredText(orbitronFont, prompt, 280,
                             GAME_OVER_FONT_SIZE - 8, DARKGRAY);
            DrawCenteredText(orbitronFont, "Press ENTER to save", 340, MESSAGE_FONT_SIZE, GRAY);
        }

        EndDrawing();
    }

    // De-Initialization
    UnloadFont(orbitronFont);
    CloseWindow();
    return 0;
}