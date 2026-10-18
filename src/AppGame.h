#pragma once
#include "App.h"
#include "UI_AA.h"

class AppGame : public App {
private:
    enum GameState { STATE_READY, STATE_PLAYING, STATE_GAME_OVER, STATE_VICTORY };
    GameState state = STATE_READY;

    // Arena dimensions (Portrait: 240 x 320)
    static const int ARENA_W = 240;
    static const int ARENA_H = 320;
    static const int BOTTOM_OFFSET = 310;

    // Paddle
    const int PADDLE_W = 48;
    const int PADDLE_H = 8;
    const int PADDLE_Y = 285;
    float paddleX = (ARENA_W - PADDLE_W) / 2.0f;
    float prevPaddleX = (ARENA_W - PADDLE_W) / 2.0f;

    // Ball
    const int BALL_RADIUS = 4;
    float ballX = 120.0f;
    float ballY = 250.0f;
    float prevBallX = 120.0f;
    float prevBallY = 250.0f;
    float ballVX = 2.0f;
    float ballVY = -3.0f;
    float baseSpeed = 3.4f;

    // Bricks: 4 rows x 6 columns
    static const int BRICK_ROWS = 4;
    static const int BRICK_COLS = 6;
    static const int BRICK_W = 35;
    static const int BRICK_H = 12;
    static const int BRICK_GAP = 3;
    static const int BRICK_START_X = 7;
    static const int BRICK_START_Y = 56;

    bool bricks[BRICK_ROWS][BRICK_COLS];
    const uint16_t rowColors[BRICK_ROWS] = {
        TFT_RED, TFT_ORANGE, TFT_GREEN, TFT_CYAN
    };

    uint16_t score = 0;
    uint16_t highScore = 0;
    int8_t lives = 3;
    int remainingBricks = 0;

    void resetGame() {
        score = 0;
        lives = 3;
        resetRound();
    }

    void resetRound() {
        paddleX = (ARENA_W - PADDLE_W) / 2.0f;
        prevPaddleX = paddleX;
        ballX = 120.0f;
        ballY = 250.0f;
        prevBallX = ballX;
        prevBallY = ballY;
        ballVX = 2.0f;
        ballVY = -baseSpeed;

        remainingBricks = BRICK_ROWS * BRICK_COLS;
        for (int r = 0; r < BRICK_ROWS; r++) {
            for (int c = 0; c < BRICK_COLS; c++) {
                bricks[r][c] = true;
            }
        }
    }

    void drawBricks(TFT_eSPI& tft) {
        for (int r = 0; r < BRICK_ROWS; r++) {
            for (int c = 0; c < BRICK_COLS; c++) {
                if (bricks[r][c]) {
                    int bx = BRICK_START_X + c * (BRICK_W + BRICK_GAP);
                    int by = BRICK_START_Y + r * (BRICK_H + BRICK_GAP);
                    UI_AA::fillRoundRect(tft, bx, by, BRICK_W, BRICK_H, 3, rowColors[r], TFT_BLACK);
                    UI_AA::drawRoundRect(tft, bx, by, BRICK_W, BRICK_H, 3, TFT_WHITE, TFT_BLACK);
                }
            }
        }
    }

    void drawHeader(TFT_eSPI& tft) {
        tft.fillRect(0, 26, ARENA_W, 24, 0x18E3);
        tft.setTextColor(TFT_WHITE, 0x18E3);
        tft.setTextDatum(TL_DATUM);
        char buf[32];
        snprintf(buf, sizeof(buf), "Score: %d", score);
        tft.drawString(buf, 8, 30, 2);

        snprintf(buf, sizeof(buf), "Hi: %d", highScore);
        tft.drawString(buf, 110, 30, 2);

        // Draw lives with anti-aliasing
        int heartX = 194;
        for (int i = 0; i < 3; i++) {
            if (i < lives) {
                UI_AA::fillCircle(tft, heartX + i * 14, 38, 4, TFT_RED, 0x18E3);
            } else {
                UI_AA::drawCircle(tft, heartX + i * 14, 38, 4, 0x52AA, 0x18E3);
            }
        }
    }

    void renderScene(TFT_eSPI& tft) {
        tft.fillRect(0, 50, ARENA_W, ARENA_H - 50, TFT_BLACK);
        drawHeader(tft);
        drawBricks(tft);

        // Paddle with anti-aliasing
        UI_AA::fillRoundRect(tft, (int)paddleX, PADDLE_Y, PADDLE_W, PADDLE_H, 3, TFT_CYAN, TFT_BLACK);
        UI_AA::drawRoundRect(tft, (int)paddleX, PADDLE_Y, PADDLE_W, PADDLE_H, 3, TFT_WHITE, TFT_BLACK);

        // Ball with anti-aliasing
        UI_AA::fillCircle(tft, (int)ballX, (int)ballY, BALL_RADIUS, TFT_YELLOW, TFT_BLACK);
    }

public:
    const char* getName() const override { return "Breakout"; }
    const char* getDescription() const override { return "Retro arcade brick breaker with touch paddle"; }
    uint16_t getThemeColor() const override { return TFT_ORANGE; }

    void onEnter(TFT_eSPI& tft) override {
        state = STATE_READY;
        resetGame();
        renderScene(tft);

        // Show Ready Prompt with anti-aliasing
        UI_AA::fillRoundRect(tft, 30, 160, 180, 50, 8, 0x001F, TFT_BLACK);
        UI_AA::drawRoundRect(tft, 30, 160, 180, 50, 8, TFT_CYAN, TFT_BLACK);
        tft.setTextColor(TFT_WHITE, 0x001F);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("TAP TO START", 120, 185, 2);
    }

    void onUpdate(TFT_eSPI& tft, const TouchPoint& touch, unsigned long dt) override {
        if (state == STATE_READY) {
            if (touch.justPressed && touch.y > 50) {
                state = STATE_PLAYING;
                renderScene(tft);
            }
            return;
        }

        if (state == STATE_GAME_OVER || state == STATE_VICTORY) {
            if (touch.justPressed && touch.y > 50) {
                resetGame();
                state = STATE_PLAYING;
                renderScene(tft);
            }
            return;
        }

        // Touch Paddle Control
        if (touch.pressed && touch.y >= 100) {
            float targetPaddleX = touch.x - (PADDLE_W / 2.0f);
            if (targetPaddleX < 2) targetPaddleX = 2;
            if (targetPaddleX > ARENA_W - PADDLE_W - 2) targetPaddleX = ARENA_W - PADDLE_W - 2;
            paddleX += (targetPaddleX - paddleX) * 0.45f;
        }

        // Ball Physics
        ballX += ballVX;
        ballY += ballVY;

        if (ballX <= BALL_RADIUS + 2) {
            ballX = BALL_RADIUS + 2;
            ballVX = -ballVX;
        } else if (ballX >= ARENA_W - BALL_RADIUS - 2) {
            ballX = ARENA_W - BALL_RADIUS - 2;
            ballVX = -ballVX;
        }

        if (ballY <= 52 + BALL_RADIUS) {
            ballY = 52 + BALL_RADIUS;
            ballVY = -ballVY;
        }

        // Paddle Collision
        if (ballY + BALL_RADIUS >= PADDLE_Y && prevBallY + BALL_RADIUS <= PADDLE_Y + 4) {
            if (ballX >= paddleX - 2 && ballX <= paddleX + PADDLE_W + 2) {
                ballY = PADDLE_Y - BALL_RADIUS;
                float hitOffset = (ballX - (paddleX + PADDLE_W / 2.0f)) / (PADDLE_W / 2.0f);
                ballVX = hitOffset * 3.8f;
                ballVY = -sqrtf(max(1.5f, baseSpeed * baseSpeed - ballVX * ballVX));
                if (fabsf(ballVY) < 1.8f) ballVY = -1.8f;
            }
        }

        // Brick Collision
        int brickRow = -1, brickCol = -1;
        for (int r = 0; r < BRICK_ROWS; r++) {
            for (int c = 0; c < BRICK_COLS; c++) {
                if (bricks[r][c]) {
                    int bx = BRICK_START_X + c * (BRICK_W + BRICK_GAP);
                    int by = BRICK_START_Y + r * (BRICK_H + BRICK_GAP);
                    if (ballX + BALL_RADIUS >= bx && ballX - BALL_RADIUS <= bx + BRICK_W &&
                        ballY + BALL_RADIUS >= by && ballY - BALL_RADIUS <= by + BRICK_H) {
                        brickRow = r;
                        brickCol = c;
                        break;
                    }
                }
            }
            if (brickRow >= 0) break;
        }

        if (brickRow >= 0) {
            bricks[brickRow][brickCol] = false;
            remainingBricks--;
            score += (BRICK_ROWS - brickRow) * 10;
            if (score > highScore) highScore = score;
            ballVY = -ballVY;

            int bx = BRICK_START_X + brickCol * (BRICK_W + BRICK_GAP);
            int by = BRICK_START_Y + brickRow * (BRICK_H + BRICK_GAP);
            tft.fillRect(bx - 1, by - 1, BRICK_W + 2, BRICK_H + 2, TFT_BLACK);
            drawHeader(tft);

            if (remainingBricks <= 0) {
                state = STATE_VICTORY;
                UI_AA::fillRoundRect(tft, 30, 150, 180, 60, 8, 0x0540, TFT_BLACK);
                UI_AA::drawRoundRect(tft, 30, 150, 180, 60, 8, TFT_GREEN, TFT_BLACK);
                tft.setTextColor(TFT_GREEN, 0x0540);
                tft.setTextDatum(MC_DATUM);
                tft.drawString("VICTORY!", 120, 170, 4);
                tft.drawString("Tap to play again", 120, 195, 2);
                return;
            }
        }

        // Ball out
        if (ballY > BOTTOM_OFFSET) {
            lives--;
            drawHeader(tft);
            if (lives <= 0) {
                state = STATE_GAME_OVER;
                UI_AA::fillRoundRect(tft, 30, 150, 180, 60, 8, 0x9000, TFT_BLACK);
                UI_AA::drawRoundRect(tft, 30, 150, 180, 60, 8, TFT_RED, TFT_BLACK);
                tft.setTextColor(TFT_RED, 0x9000);
                tft.setTextDatum(MC_DATUM);
                tft.drawString("GAME OVER", 120, 170, 4);
                tft.drawString("Tap to try again", 120, 195, 2);
                return;
            } else {
                tft.fillCircle((int)prevBallX, (int)prevBallY, BALL_RADIUS + 2, TFT_BLACK);
                tft.fillRect((int)prevPaddleX - 1, PADDLE_Y - 1, PADDLE_W + 2, PADDLE_H + 2, TFT_BLACK);
                paddleX = (ARENA_W - PADDLE_W) / 2.0f;
                ballX = 120.0f;
                ballY = 250.0f;
                prevPaddleX = paddleX;
                prevBallX = ballX;
                prevBallY = ballY;
                ballVX = 2.0f;
                ballVY = -baseSpeed;
                UI_AA::fillRoundRect(tft, (int)paddleX, PADDLE_Y, PADDLE_W, PADDLE_H, 3, TFT_CYAN, TFT_BLACK);
                UI_AA::drawRoundRect(tft, (int)paddleX, PADDLE_Y, PADDLE_W, PADDLE_H, 3, TFT_WHITE, TFT_BLACK);
                UI_AA::fillCircle(tft, (int)ballX, (int)ballY, BALL_RADIUS, TFT_YELLOW, TFT_BLACK);
            }
        }

        // Differential 60FPS Refresh with Anti-Aliasing
        if ((int)paddleX != (int)prevPaddleX) {
            tft.fillRect((int)prevPaddleX - 1, PADDLE_Y - 1, PADDLE_W + 2, PADDLE_H + 2, TFT_BLACK);
            UI_AA::fillRoundRect(tft, (int)paddleX, PADDLE_Y, PADDLE_W, PADDLE_H, 3, TFT_CYAN, TFT_BLACK);
            UI_AA::drawRoundRect(tft, (int)paddleX, PADDLE_Y, PADDLE_W, PADDLE_H, 3, TFT_WHITE, TFT_BLACK);
            prevPaddleX = paddleX;
        }

        tft.fillCircle((int)prevBallX, (int)prevBallY, BALL_RADIUS + 2, TFT_BLACK);
        UI_AA::fillCircle(tft, (int)ballX, (int)ballY, BALL_RADIUS, TFT_YELLOW, TFT_BLACK);
        prevBallX = ballX;
        prevBallY = ballY;
    }

    void onExit(TFT_eSPI& tft) override {}
};
