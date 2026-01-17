#include <raylib.h>
#include <stdio.h>
#include "ball.h"
#include "paddle.h"

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 600
#define PADDLE_WIDTH 15.0f
#define PADDLE_HEIGHT 100.0f
#define POINTS_TO_WIN 5

typedef enum {
    PLAYING,
    PLAYER_WINS,
    AI_WINS
} GameState;

static void ResetBall(Ball *ball, int screenWidth, int screenHeight, float speedMultiplier)
{
    ball->position.x = (float)screenWidth / 2.0f;
    ball->position.y = (float)screenHeight / 2.0f;
    ball->velocity.x = 4.0f * speedMultiplier;
    ball->velocity.y = 2.0f * speedMultiplier;
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

    // Load custom font
    Font orbitronFont = LoadFontEx("../resources/orbitron/Orbitron-VariableFont_wght.ttf", 32, 0, 0);

    // Initialize ball
    Ball ball = {
        .position = { (float)SCREEN_WIDTH / 2.0f, (float)SCREEN_HEIGHT / 2.0f },
        .velocity = { 4.0f, 2.0f },
        .radius = 8.0f
    };

    // Initialize player paddle (left side)
    Paddle player = {
        .position = { 20.0f, (float)(SCREEN_HEIGHT - PADDLE_HEIGHT) / 2.0f },
        .width = PADDLE_WIDTH,
        .height = PADDLE_HEIGHT,
        .velocity = 0.0f,
        .score = 0
    };

    // Initialize AI paddle (right side)
    Paddle ai = {
        .position = { (float)SCREEN_WIDTH - PADDLE_WIDTH - 20.0f, (float)(SCREEN_HEIGHT - PADDLE_HEIGHT) / 2.0f },
        .width = PADDLE_WIDTH,
        .height = PADDLE_HEIGHT,
        .velocity = 0.0f,
        .score = 0
    };

    GameState gameState = PLAYING;
    float ballSpeedMultiplier = 1.0f;

    // Main game loop
    while (!WindowShouldClose())
    {
        // Update
        if (gameState == PLAYING)
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
                ballSpeedMultiplier = 1.0f + (ai.score + player.score) * 0.02f;  // Increase speed
                if (ai.score >= POINTS_TO_WIN) {
                    gameState = AI_WINS;
                } else {
                    ResetBall(&ball, SCREEN_WIDTH, SCREEN_HEIGHT, ballSpeedMultiplier);
                }
            } else if (ball.position.x > SCREEN_WIDTH) {
                // Player scores
                player.score++;
                ballSpeedMultiplier = 1.0f + (ai.score + player.score) * 0.02f;  // Increase speed
                if (player.score >= POINTS_TO_WIN) {
                    gameState = PLAYER_WINS;
                } else {
                    ResetBall(&ball, SCREEN_WIDTH, SCREEN_HEIGHT, ballSpeedMultiplier);
                }
            }
        }

        // Handle game over and restart
        if ((gameState == PLAYER_WINS || gameState == AI_WINS) && IsKeyPressed(KEY_SPACE)) {
            gameState = PLAYING;
            player.score = 0;
            ai.score = 0;
            ballSpeedMultiplier = 1.0f;
            ResetBall(&ball, SCREEN_WIDTH, SCREEN_HEIGHT, ballSpeedMultiplier);
            player.position.y = (float)(SCREEN_HEIGHT - PADDLE_HEIGHT) / 2.0f;
            ai.position.y = (float)(SCREEN_HEIGHT - PADDLE_HEIGHT) / 2.0f;
        }

        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw center line
        for (int i = 0; i < SCREEN_HEIGHT; i += 20) {
            DrawLineV((Vector2){ SCREEN_WIDTH / 2.0f, (float)i }, 
                      (Vector2){ SCREEN_WIDTH / 2.0f, (float)(i + 10) }, LIGHTGRAY);
        }

        // Draw title
        DrawCenteredText(orbitronFont, "PONG", 10, 48, DARKGRAY);

        // Draw paddles
        DrawPaddle(&player, BLUE);
        DrawPaddle(&ai, RED);

        // Draw ball
        DrawCircleV(ball.position, ball.radius, PURPLE);

        // Draw scores
        char playerScoreText[20], aiScoreText[20];
        snprintf(playerScoreText, sizeof(playerScoreText), "Player: %d", player.score);
        snprintf(aiScoreText, sizeof(aiScoreText), "AI: %d", ai.score);

        DrawTextEx(orbitronFont, playerScoreText, (Vector2){50, 80}, 28, 1, BLUE);
        DrawTextEx(orbitronFont, aiScoreText, (Vector2){SCREEN_WIDTH - 250, 80}, 28, 1, RED);

        // Draw FPS
        DrawFPS(10, 10);

        // Draw game state messages
        if (gameState == PLAYER_WINS) {
            DrawCenteredText(orbitronFont, "YOU WIN!", 250, 40, GREEN);
            DrawCenteredText(orbitronFont, "Press SPACE to play again", 310, 24, GRAY);
        } else if (gameState == AI_WINS) {
            DrawCenteredText(orbitronFont, "AI WINS!", 250, 40, RED);
            DrawCenteredText(orbitronFont, "Press SPACE to play again", 310, 24, GRAY);
        }

        EndDrawing();
    }

    // De-Initialization
    UnloadFont(orbitronFont);
    CloseWindow();
    return 0;
}