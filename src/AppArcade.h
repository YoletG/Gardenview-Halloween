#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "App.h"
#include "AppGame.h"
#include "AppGourdy.h"
#include "UI_AA.h"

class AppArcade : public App {
public:
    enum SubGame {
        GAME_MENU = 0,
        GAME_GOURDY,
        GAME_BREAKOUT
    };

private:
    SubGame activeGame = GAME_MENU;
    AppGame breakout;
    AppGourdy gourdy;

    unsigned long cooldownUntil = 0;

    void drawArcadeMenu(TFT_eSPI& tft) {
        tft.fillScreen(TFT_BLACK);

        // Header Sub-bar
        tft.fillRect(0, 26, 240, 36, 0x1082);
        tft.drawFastHLine(0, 62, 240, 0x3186);
        tft.setTextColor(TFT_WHITE, 0x1082);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("GAMES ARCADE", 120, 36, 2);
        tft.setTextColor(0x94B2, 0x1082);
        tft.drawString("Choose an arcade game to play", 120, 50, 1);

        // Card 1: Gardenview Halloween (Dandy's World)
        int card1Y = 72;
        int cardH = 100;
        UI_AA::fillRoundRect(tft, 8, card1Y, 224, cardH, 8, 0x18C3, TFT_BLACK);
        UI_AA::drawRoundRect(tft, 8, card1Y, 224, cardH, 8, TFT_ORANGE, TFT_BLACK);

        // Pumpkin Icon Accent Box
        UI_AA::fillRoundRect(tft, 14, card1Y + 12, 54, 76, 6, 0x2A15, 0x18C3);
        UI_AA::drawRoundRect(tft, 14, card1Y + 12, 54, 76, 6, TFT_ORANGE, 0x18C3);

        // Cute Mini Pumpkin on Card
        int px = 41, py = card1Y + 50;
        UI_AA::fillCircle(tft, px - 5, py, 9, 0xFA40, 0x2A15);
        UI_AA::fillCircle(tft, px + 5, py, 9, 0xFA40, 0x2A15);
        UI_AA::fillCircle(tft, px, py, 10, 0xFD20, 0x2A15);
        tft.fillRect(px - 1, py - 14, 3, 4, 0x69A1); // Stem
        tft.fillRect(px + 2, py - 14, 4, 2, 0x24C5); // Leaf
        tft.fillRect(px - 5, py - 4, 3, 3, TFT_BLACK); // Eyes
        tft.fillRect(px + 2, py - 4, 3, 3, TFT_BLACK);
        tft.drawFastHLine(px - 4, py + 4, 9, TFT_BLACK); // Grin
        tft.drawPixel(px - 2, py + 4, TFT_WHITE); // Fangs
        tft.drawPixel(px + 2, py + 4, TFT_WHITE);

        // Titles & Description
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_ORANGE, 0x18C3);
        tft.drawString("Gardenview", 74, card1Y + 12, 4);

        tft.setTextColor(TFT_YELLOW, 0x18C3);
        tft.drawString("Halloween", 74, card1Y + 34, 4);

        tft.setTextColor(0xD81F, 0x18C3);
        tft.drawString("Dandy's World Survival", 74, card1Y + 58, 2);

        tft.setTextColor(0xAD75, 0x18C3);
        tft.drawString("Play as Gourdy! Collect Ichor,", 74, card1Y + 76, 1);
        tft.drawString("extract & escape Twisteds!", 74, card1Y + 87, 1);

        // Card 2: Breakout Arcade
        int card2Y = 186;
        UI_AA::fillRoundRect(tft, 8, card2Y, 224, cardH, 8, 0x18C3, TFT_BLACK);
        UI_AA::drawRoundRect(tft, 8, card2Y, 224, cardH, 8, TFT_CYAN, TFT_BLACK);

        // Paddle & Ball Accent Box
        UI_AA::fillRoundRect(tft, 14, card2Y + 12, 54, 76, 6, 0x0A25, 0x18C3);
        UI_AA::drawRoundRect(tft, 14, card2Y + 12, 54, 76, 6, TFT_CYAN, 0x18C3);

        // Mini Breakout illustration
        int bx = 41, by = card2Y + 50;
        // Bricks
        tft.fillRect(bx - 16, by - 22, 14, 6, TFT_RED);
        tft.fillRect(bx + 2, by - 22, 14, 6, TFT_GREEN);
        tft.fillRect(bx - 16, by - 13, 14, 6, TFT_ORANGE);
        tft.fillRect(bx + 2, by - 13, 14, 6, TFT_CYAN);
        // Yellow Ball
        UI_AA::fillCircle(tft, bx + 2, by - 2, 3, TFT_YELLOW, 0x0A25);
        // Cyan Paddle
        UI_AA::fillRoundRect(tft, bx - 16, by + 16, 32, 6, 2, TFT_CYAN, 0x0A25);

        // Titles & Description
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_CYAN, 0x18C3);
        tft.drawString("Breakout", 74, card2Y + 14, 4);

        tft.setTextColor(TFT_WHITE, 0x18C3);
        tft.drawString("Retro Brick Breaker", 74, card2Y + 40, 2);

        tft.setTextColor(0xAD75, 0x18C3);
        tft.drawString("Touch paddle, 60 FPS physics", 74, card2Y + 60, 1);
        tft.drawString("& high score arcade action", 74, card2Y + 74, 1);

        // Footer Hint
        tft.fillRect(0, 304, 240, 16, 0x0841);
        tft.setTextColor(0x5AEB, 0x0841);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("Tap any game to play", 120, 312, 1);
    }

public:
    AppArcade() {}

    const char* getName() const override { return "Arcade"; }
    const char* getDescription() const override { return "Gourdy's Escape & Breakout games"; }
    uint16_t getThemeColor() const override { return TFT_ORANGE; }

    void onEnter(TFT_eSPI& tft) override {
        activeGame = GAME_MENU;
        cooldownUntil = millis() + 300;
        drawArcadeMenu(tft);
    }

    void onUpdate(TFT_eSPI& tft, const TouchPoint& touch, unsigned long dt) override {
        if (millis() < cooldownUntil) return;

        // If in Menu
        if (activeGame == GAME_MENU) {
            if (touch.justPressed) {
                // Card 1: Gourdy's Escape (y: 72..172)
                if (touch.x >= 8 && touch.x <= 232 && touch.y >= 72 && touch.y <= 172) {
                    activeGame = GAME_GOURDY;
                    cooldownUntil = millis() + 300;
                    tft.fillScreen(TFT_BLACK);
                    gourdy.onEnter(tft);
                    return;
                }
                // Card 2: Breakout (y: 186..286)
                if (touch.x >= 8 && touch.x <= 232 && touch.y >= 186 && touch.y <= 286) {
                    activeGame = GAME_BREAKOUT;
                    cooldownUntil = millis() + 300;
                    tft.fillScreen(TFT_BLACK);
                    breakout.onEnter(tft);
                    return;
                }
            }
            return;
        }

        // Subgame Navigation: Check "< ARCADE" back button (x: 4..56, y: 26..48)
        if (touch.justPressed && touch.x >= 4 && touch.x <= 56 && touch.y >= 26 && touch.y <= 48) {
            if (activeGame == GAME_GOURDY) gourdy.onExit(tft);
            else if (activeGame == GAME_BREAKOUT) breakout.onExit(tft);

            activeGame = GAME_MENU;
            cooldownUntil = millis() + 300;
            drawArcadeMenu(tft);
            return;
        }

        // Forward updates to active subgame
        if (activeGame == GAME_GOURDY) {
            gourdy.onUpdate(tft, touch, dt);
        } else if (activeGame == GAME_BREAKOUT) {
            breakout.onUpdate(tft, touch, dt);
        }
    }

    void onExit(TFT_eSPI& tft) override {
        if (activeGame == GAME_GOURDY) gourdy.onExit(tft);
        else if (activeGame == GAME_BREAKOUT) breakout.onExit(tft);
        activeGame = GAME_MENU;
    }
};
