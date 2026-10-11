#pragma once
#include <Arduino.h>
#include <vector>
#include <TFT_eSPI.h>
#include <xpt2046.h>
#include "App.h"
#include "pins.h"
#include "UI_AA.h"

class AppManager {
private:
    TFT_eSPI& tft;
    XPT2046& touch;
    std::vector<App*> apps;
    int activeAppIndex = -1; // -1 = Home Launcher

    unsigned long lastStatusBarUpdate = 0;
    const unsigned long STATUS_BAR_INTERVAL = 1500;
    int lastReportedPct = -1;

    unsigned long navigationCooldownUntil = 0;
    TouchPoint lastTouchState = { false, false, false, 0, 0, 0, 0 };

    void drawStatusBar(bool fullRedraw = false) {
        if (fullRedraw) {
            tft.fillRect(0, 0, 240, 25, 0x0841); // Deep Navy
            tft.drawFastHLine(0, 25, 240, 0x2965);

            if (activeAppIndex >= 0) {
                // Back button with smooth AA edges
                UI_AA::fillRoundRect(tft, 4, 2, 58, 21, 4, 0x2124, 0x0841);
                UI_AA::drawRoundRect(tft, 4, 2, 58, 21, 4, TFT_CYAN, 0x0841);
                tft.setTextColor(TFT_CYAN, 0x2124);
                tft.setTextDatum(MC_DATUM);
                tft.drawString("< HOME", 33, 12, 2);

                // App Title
                tft.setTextColor(TFT_WHITE, 0x0841);
                tft.drawString(apps[activeAppIndex]->getName(), 125, 12, 2);
            } else {
                // Launcher Title
                tft.setTextColor(TFT_CYAN, 0x0841);
                tft.setTextDatum(ML_DATUM);
                tft.drawString("LILYGO T-HMI", 8, 12, 2);
            }
        }

        // Battery level update without full screen wipe
        uint32_t mv = analogReadMilliVolts(BAT_ADC_PIN) * 2;
        float volts = mv / 1000.0f;
        int pct = 0;
        if (volts >= 4.20f) pct = 100;
        else if (volts <= 3.30f) pct = 0;
        else pct = (int)((volts - 3.30f) / (4.20f - 3.30f) * 100.0f);

        if (fullRedraw || pct != lastReportedPct) {
            lastReportedPct = pct;
            tft.fillRect(168, 2, 68, 21, 0x0841);
            tft.setTextDatum(MR_DATUM);
            char batBuf[16];
            if (volts > 4.30f) {
                snprintf(batBuf, sizeof(batBuf), "USB [=]");
                tft.setTextColor(TFT_GREEN, 0x0841);
            } else {
                snprintf(batBuf, sizeof(batBuf), "%d%%", pct);
                tft.setTextColor(pct > 20 ? TFT_WHITE : TFT_RED, 0x0841);
            }
            tft.drawString(batBuf, 234, 12, 2);
        }
    }

    void drawLauncher() {
        tft.fillScreen(TFT_BLACK);
        drawStatusBar(true);

        // Header
        tft.fillRect(0, 26, 240, 36, 0x1082);
        tft.drawFastHLine(0, 62, 240, 0x3186);
        tft.setTextColor(TFT_WHITE, 0x1082);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("APPLICATION LAUNCHER", 120, 36, 2);
        tft.setTextColor(0x94B2, 0x1082);
        tft.drawString("Tap any application to launch", 120, 50, 1);

        // Cards (Portrait 240 x 320 layout)
        int cardY = 68;
        int cardH = 68;
        int cardSpacing = 10;

        for (size_t i = 0; i < apps.size(); i++) {
            App* app = apps[i];
            int y = cardY + i * (cardH + cardSpacing);
            uint16_t themeCol = app->getThemeColor();

            // Card background & border with Anti-Aliasing
            UI_AA::fillRoundRect(tft, 8, y, 224, cardH, 8, 0x18C3, TFT_BLACK);
            UI_AA::drawRoundRect(tft, 8, y, 224, cardH, 8, themeCol, TFT_BLACK);

            // Icon Accent Box
            UI_AA::fillRoundRect(tft, 14, y + 8, 46, 52, 6, themeCol, 0x18C3);
            tft.setTextColor(TFT_BLACK, themeCol);
            tft.setTextDatum(MC_DATUM);
            char numBuf[4];
            snprintf(numBuf, sizeof(numBuf), "#%d", (int)i + 1);
            tft.drawString(numBuf, 37, y + 34, 4);

            // Title & Description
            tft.setTextColor(TFT_WHITE, 0x18C3);
            tft.setTextDatum(TL_DATUM);
            tft.drawString(app->getName(), 68, y + 10, 4);

            tft.setTextColor(0xAD75, 0x18C3);
            tft.drawString(app->getDescription(), 68, y + 38, 1);

            // Right Chevron '>'
            tft.setTextColor(themeCol, 0x18C3);
            tft.setTextDatum(MC_DATUM);
            tft.drawString(">", 216, y + 34, 4);
        }

        // Footer
        tft.fillRect(0, 304, 240, 16, 0x0841);
        tft.setTextColor(0x5AEB, 0x0841);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("Press BOOT button anytime to return Home", 120, 312, 1);
    }

public:
    AppManager(TFT_eSPI& display, XPT2046& touchController)
        : tft(display), touch(touchController) {}

    void registerApp(App* app) {
        apps.push_back(app);
    }

    void begin() {
        navigationCooldownUntil = millis() + 400;
        drawLauncher();
    }

    void launchApp(int index) {
        if (index < 0 || index >= (int)apps.size()) return;
        activeAppIndex = index;
        navigationCooldownUntil = millis() + 400;
        tft.fillScreen(TFT_BLACK);
        drawStatusBar(true);
        apps[activeAppIndex]->onEnter(tft);
    }

    void returnHome() {
        if (activeAppIndex >= 0) {
            apps[activeAppIndex]->onExit(tft);
            activeAppIndex = -1;
            navigationCooldownUntil = millis() + 400;
            drawLauncher();
        }
    }

    void update(unsigned long dt) {
        // Hardware Home Key: BOOT button (GPIO 0)
        if (digitalRead(BUTTON1_PIN) == LOW) {
            static unsigned long lastBtnPress = 0;
            if (millis() - lastBtnPress > 350) {
                lastBtnPress = millis();
                if (activeAppIndex >= 0) {
                    returnHome();
                    return;
                }
            }
        }

        // Sample Touch Sensor
        bool isPressed = touch.pressed();
        TouchPoint currentTouch = { false, false, false, 0, 0, 0, 0 };

        if (isPressed) {
            uint16_t rx = touch.RawX();
            uint16_t ry = touch.RawY();
            if (rx >= 50 && rx <= 4000 && ry >= 50 && ry <= 4000) {
                currentTouch.pressed = true;
                currentTouch.x = touch.X();
                currentTouch.y = touch.Y();
                currentTouch.rawX = rx;
                currentTouch.rawY = ry;
            }
        }

        currentTouch.justPressed = (currentTouch.pressed && !lastTouchState.pressed);
        currentTouch.justReleased = (!currentTouch.pressed && lastTouchState.pressed);

        if (currentTouch.justPressed) {
            Serial.printf("[TOUCH] raw(%d,%d) -> screen(%d,%d)\n", currentTouch.rawX, currentTouch.rawY, currentTouch.x, currentTouch.y);
        }

        // Status Bar periodic update (no screen flashing)
        if (millis() - lastStatusBarUpdate >= STATUS_BAR_INTERVAL) {
            lastStatusBarUpdate = millis();
            drawStatusBar(false);
        }

        // Cooldown between screen transitions
        if (millis() < navigationCooldownUntil) {
            lastTouchState = currentTouch;
            return;
        }

        // Home Launcher Navigation
        if (activeAppIndex < 0) {
            if (currentTouch.justPressed && currentTouch.x > 0 && currentTouch.y > 0) {
                int cardY = 68;
                int cardH = 68;
                int cardSpacing = 10;

                for (size_t i = 0; i < apps.size(); i++) {
                    int y = cardY + i * (cardH + cardSpacing);
                    if (currentTouch.x >= 8 && currentTouch.x <= 232 &&
                        currentTouch.y >= y && currentTouch.y <= y + cardH) {
                        launchApp(i);
                        lastTouchState = currentTouch;
                        return;
                    }
                }
            }
        } else {
            // Running App: Check Top "< HOME" button tap (x: 4..64, y: 2..24)
            if (currentTouch.justPressed && currentTouch.x >= 4 && currentTouch.x <= 64 &&
                currentTouch.y >= 2 && currentTouch.y <= 24) {
                returnHome();
                lastTouchState = currentTouch;
                return;
            }

            // Forward update to active app
            apps[activeAppIndex]->onUpdate(tft, currentTouch, dt);
        }

        lastTouchState = currentTouch;
    }
};
