#pragma once
#include "App.h"
#include "UI_AA.h"
#include <esp_heap_caps.h>
#include <math.h>

class AppDrawingPad : public App {
private:
    uint16_t currentColor = TFT_WHITE;
    bool isEraser = false;
    uint8_t brushSize = 4; // 1 to 100 px!

    int16_t lastX = -1;
    int16_t lastY = -1;
    unsigned long lastContactTime = 0;
    const unsigned long STROKE_HOLD_MS = 60; // Hysteresis to maintain unbroken lines during light stylus pressure

    // Canvas boundaries (Portrait: 240 x 320)
    const int16_t CANVAS_LEFT = 2;
    const int16_t CANVAS_RIGHT = 237;
    const int16_t CANVAS_TOP = 56;
    const int16_t CANVAS_BOTTOM = 282;

    // Color Picker Modal State
    bool colorPickerOpen = false;
    bool sizePickerOpen = false;

    // IbisPaint HSV values
    uint8_t currentHue = 0;        // 0..255 (0..360 deg)
    uint8_t currentSat = 255;      // 0..255
    uint8_t currentVal = 255;      // 0..255

    // In-memory 16-bit framebuffer in 8MB PSRAM for canvas persistence across apps & modals
    uint16_t* canvasBuf = nullptr;
    bool hasDrawnBefore = false;

    // Helper: HSV to RGB565 conversion
    uint16_t hsvToRgb565(uint8_t h, uint8_t s, uint8_t v) {
        uint8_t r, g, b;
        if (s == 0) {
            r = g = b = v;
        } else {
            uint8_t region = h / 43;
            uint8_t remainder = (h - (region * 43)) * 6;
            uint8_t p = (v * (255 - s)) >> 8;
            uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
            uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;
            switch (region) {
                case 0: r = v; g = t; b = p; break;
                case 1: r = q; g = v; b = p; break;
                case 2: r = p; g = v; b = t; break;
                case 3: r = p; g = q; b = v; break;
                case 4: r = t; g = p; b = v; break;
                default: r = v; g = p; b = q; break;
            }
        }
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }

    // =========================================================================
    // SOLID CANVAS DRAWING (CRISP INTERSECTIONS & SMART CLOSE-POINT REDRAW)
    // =========================================================================
    void drawSolidCircle(TFT_eSPI& tft, int cx, int cy, int radius, uint16_t color) {
        if (radius <= 1) {
            if (cx >= CANVAS_LEFT && cx <= CANVAS_RIGHT && cy >= CANVAS_TOP && cy <= CANVAS_BOTTOM) {
                if (canvasBuf) canvasBuf[cy * 240 + cx] = color;
                tft.drawPixel(cx, cy, color);
            }
            return;
        }

        int minPy = max((int)CANVAS_TOP, cy - radius);
        int maxPy = min((int)CANVAS_BOTTOM, cy + radius);
        int rSq = radius * radius;

        for (int py = minPy; py <= maxPy; py++) {
            int dy = py - cy;
            int dxMax = (int)sqrtf((float)(rSq - dy * dy));
            int x1 = max((int)CANVAS_LEFT, cx - dxMax);
            int x2 = min((int)CANVAS_RIGHT, cx + dxMax);
            if (x2 >= x1) {
                int len = x2 - x1 + 1;
                if (canvasBuf) {
                    uint16_t* rowPtr = &canvasBuf[py * 240 + x1];
                    for (int k = 0; k < len; k++) rowPtr[k] = color;
                }
                tft.drawFastHLine(x1, py, len, color);
            }
        }
    }

    void drawStrokeSegment(TFT_eSPI& tft, int x0, int y0, int x1, int y1, int radius, uint16_t color) {
        int dx = x1 - x0;
        int dy = y1 - y0;
        float dist = sqrtf((float)(dx * dx + dy * dy));

        if (dist < 1.0f) {
            drawSolidCircle(tft, x1, y1, radius, color);
            return;
        }

        if (radius <= 1) {
            int steps = (int)ceilf(dist);
            for (int s = 0; s <= steps; s++) {
                int px = x0 + (dx * s) / steps;
                int py = y0 + (dy * s) / steps;
                if (px >= CANVAS_LEFT && px <= CANVAS_RIGHT && py >= CANVAS_TOP && py <= CANVAS_BOTTOM) {
                    if (canvasBuf) canvasBuf[py * 240 + px] = color;
                    tft.drawPixel(px, py, color);
                }
            }
            return;
        }

        // Thick brush: interpolate solid circles with step size = max(1, radius / 2)
        // Ensures continuous solid line without gaps, and perfectly clean stroke intersections
        float stepSize = max(1.0f, (float)radius * 0.5f);
        int steps = (int)ceilf(dist / stepSize);

        for (int s = 0; s <= steps; s++) {
            float t = (float)s / (float)steps;
            int px = (int)(x0 + dx * t + 0.5f);
            int py = (int)(y0 + dy * t + 0.5f);
            drawSolidCircle(tft, px, py, radius, color);
        }
    }

    // =========================================================================
    // ANTI-ALIASED UI TOOLBAR (Background: 0x18E3)
    // =========================================================================
    void drawToolbar(TFT_eSPI& tft) {
        const uint16_t bg = 0x18E3;
        tft.fillRect(0, 26, 240, 28, bg);
        tft.drawFastHLine(0, 54, 240, 0x4208);

        // 1. Tool Toggle (PEN / ERASE)
        uint16_t btnCol = isEraser ? TFT_RED : 0x037A;
        UI_AA::fillRoundRect(tft, 4, 28, 52, 24, 4, btnCol, bg);
        UI_AA::drawRoundRect(tft, 4, 28, 52, 24, 4, TFT_WHITE, bg);
        tft.setTextColor(TFT_WHITE, btnCol);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(isEraser ? "ERASE" : "PEN", 30, 40, 2);

        // 2. Brush Size Button
        UI_AA::fillRoundRect(tft, 60, 28, 54, 24, 4, 0x2965, bg);
        UI_AA::drawRoundRect(tft, 60, 28, 54, 24, 4, TFT_CYAN, bg);
        tft.setTextColor(TFT_CYAN, 0x2965);
        char szBuf[12];
        snprintf(szBuf, sizeof(szBuf), "%dpx", brushSize);
        tft.drawString(szBuf, 87, 40, 2);

        // 3. Quick Size Steppers [-] and [+]
        UI_AA::fillRoundRect(tft, 118, 28, 26, 24, 4, 0x2124, bg);
        UI_AA::drawRoundRect(tft, 118, 28, 26, 24, 4, TFT_WHITE, bg);
        tft.setTextColor(TFT_WHITE, 0x2124);
        tft.drawString("-", 131, 40, 2);

        UI_AA::fillRoundRect(tft, 148, 28, 26, 24, 4, 0x2124, bg);
        UI_AA::drawRoundRect(tft, 148, 28, 26, 24, 4, TFT_WHITE, bg);
        tft.setTextColor(TFT_WHITE, 0x2124);
        tft.drawString("+", 161, 40, 2);

        // 4. Clear Canvas Button
        UI_AA::fillRoundRect(tft, 178, 28, 58, 24, 4, 0x8800, bg);
        UI_AA::drawRoundRect(tft, 178, 28, 58, 24, 4, TFT_WHITE, bg);
        tft.setTextColor(TFT_WHITE, 0x8800);
        tft.drawString("CLEAR", 207, 40, 2);
    }

    // =========================================================================
    // ANTI-ALIASED UI BOTTOM BAR (Background: 0x1082)
    // =========================================================================
    void drawBottomBar(TFT_eSPI& tft) {
        const uint16_t bg = 0x1082;
        tft.fillRect(0, 286, 240, 34, bg);
        tft.drawFastHLine(0, 285, 240, 0x3186);

        // IbisPaint Active Color Wheel Swatch with Anti-Aliased Rings
        uint16_t activeCol = isEraser ? TFT_BLACK : currentColor;
        UI_AA::fillCircle(tft, 18, 302, 12, activeCol, bg);
        UI_AA::drawCircle(tft, 18, 302, 13, TFT_WHITE, bg);
        UI_AA::drawCircle(tft, 18, 302, 14, TFT_CYAN, bg);

        // Quick Swatches with Anti-Aliasing (White, Red, Yellow, Green, Cyan, Blue)
        const uint16_t quickPal[6] = { TFT_WHITE, TFT_RED, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_BLUE };
        for (int i = 0; i < 6; i++) {
            int cx = 46 + i * 22;
            UI_AA::fillCircle(tft, cx, 302, 8, quickPal[i], bg);
            UI_AA::drawCircle(tft, cx, 302, 9, 0x52AA, bg);
        }

        // Palette Modal Prompt Button
        UI_AA::fillRoundRect(tft, 180, 290, 56, 24, 4, 0x3186, bg);
        UI_AA::drawRoundRect(tft, 180, 290, 56, 24, 4, TFT_YELLOW, bg);
        tft.setTextColor(TFT_YELLOW, 0x3186);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("COLOR", 208, 302, 1);
    }

    void clearCanvas(TFT_eSPI& tft) {
        hasDrawnBefore = false;
        tft.fillRect(CANVAS_LEFT, CANVAS_TOP, CANVAS_RIGHT - CANVAS_LEFT + 1, CANVAS_BOTTOM - CANVAS_TOP + 1, TFT_BLACK);
        tft.drawRect(CANVAS_LEFT - 1, CANVAS_TOP - 1, (CANVAS_RIGHT - CANVAS_LEFT) + 3, (CANVAS_BOTTOM - CANVAS_TOP) + 3, 0x2124);
        if (canvasBuf) {
            for (int y = CANVAS_TOP; y <= CANVAS_BOTTOM; y++) {
                memset(&canvasBuf[y * 240 + CANVAS_LEFT], 0, (CANVAS_RIGHT - CANVAS_LEFT + 1) * sizeof(uint16_t));
            }
        }
    }

    void redrawCanvas(TFT_eSPI& tft) {
        tft.fillRect(CANVAS_LEFT, CANVAS_TOP, CANVAS_RIGHT - CANVAS_LEFT + 1, CANVAS_BOTTOM - CANVAS_TOP + 1, TFT_BLACK);
        tft.drawRect(CANVAS_LEFT - 1, CANVAS_TOP - 1, (CANVAS_RIGHT - CANVAS_LEFT) + 3, (CANVAS_BOTTOM - CANVAS_TOP) + 3, 0x2124);
        if (canvasBuf) {
            int canvasW = CANVAS_RIGHT - CANVAS_LEFT + 1;
            for (int y = CANVAS_TOP; y <= CANVAS_BOTTOM; y++) {
                tft.pushImage(CANVAS_LEFT, y, canvasW, 1, &canvasBuf[y * 240 + CANVAS_LEFT]);
            }
        }
    }

    // =========================================================================
    // IBISPAINT STYLE COLOR PICKER MODAL (Background: 0x1082)
    // =========================================================================
    void drawColorPickerModal(TFT_eSPI& tft) {
        const uint16_t bg = 0x1082;
        tft.fillRect(0, 26, 240, 294, bg);
        tft.drawRect(0, 26, 240, 294, TFT_CYAN);

        tft.setTextColor(TFT_WHITE, bg);
        tft.setTextDatum(TC_DATUM);
        tft.drawString("IBISPAINT COLOR PICKER", 120, 32, 2);

        // 1. Rainbow Hue Bar (x: 10..230, y: 56..80)
        tft.drawRect(9, 55, 222, 26, TFT_WHITE);
        for (int x = 10; x <= 229; x++) {
            uint8_t h = ((x - 10) * 255) / 219;
            uint16_t col = hsvToRgb565(h, 255, 255);
            tft.drawFastVLine(x, 56, 24, col);
        }
        int hueIndicatorX = 10 + (currentHue * 219) / 255;
        tft.drawRect(hueIndicatorX - 2, 54, 5, 28, TFT_WHITE);
        tft.drawRect(hueIndicatorX - 3, 53, 7, 30, TFT_BLACK);

        // 2. Saturation & Brightness Box (x: 10..230, y: 92..184)
        tft.drawRect(9, 91, 222, 94, TFT_WHITE);
        for (int y = 92; y < 184; y += 4) {
            uint8_t v = 255 - ((y - 92) * 255) / 92;
            for (int x = 10; x < 230; x += 4) {
                uint8_t s = ((x - 10) * 255) / 220;
                uint16_t col = hsvToRgb565(currentHue, s, v);
                tft.fillRect(x, y, 4, 4, col);
            }
        }
        int satX = 10 + (currentSat * 219) / 255;
        int valY = 184 - (currentVal * 92) / 255;
        UI_AA::drawCircle(tft, satX, valY, 4, TFT_WHITE, bg);
        UI_AA::drawCircle(tft, satX, valY, 5, TFT_BLACK, bg);

        // 3. Color Preview & Hex Swatch
        uint16_t previewCol = hsvToRgb565(currentHue, currentSat, currentVal);
        UI_AA::fillRoundRect(tft, 10, 194, 80, 40, 5, previewCol, bg);
        UI_AA::drawRoundRect(tft, 10, 194, 80, 40, 5, TFT_WHITE, bg);

        // 4. Preset Swatches (x: 98..230, y: 194..234)
        const uint16_t pal[8] = { TFT_WHITE, TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_BLUE, TFT_MAGENTA };
        for (int i = 0; i < 8; i++) {
            int px = 100 + (i % 4) * 32;
            int py = (i < 4) ? 194 : 216;
            UI_AA::fillRoundRect(tft, px, py, 28, 18, 3, pal[i], bg);
            UI_AA::drawRoundRect(tft, px, py, 28, 18, 3, (previewCol == pal[i]) ? TFT_WHITE : 0x4208, bg);
        }

        // 5. Apply Button (x: 16..224, y: 248..286)
        UI_AA::fillRoundRect(tft, 16, 248, 208, 38, 5, TFT_GREEN, bg);
        UI_AA::drawRoundRect(tft, 16, 248, 208, 38, 5, TFT_WHITE, bg);
        tft.setTextColor(TFT_BLACK, TFT_GREEN);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("APPLY COLOR", 120, 267, 4);

        // Close 'X'
        tft.setTextColor(TFT_RED, bg);
        tft.drawString("X", 224, 38, 4);
    }

    // =========================================================================
    // BRUSH SIZE PICKER MODAL (Background: 0x1082)
    // =========================================================================
    void drawSizePickerModal(TFT_eSPI& tft) {
        const uint16_t bg = 0x1082;
        tft.fillRect(0, 26, 240, 294, bg);
        tft.drawRect(0, 26, 240, 294, TFT_CYAN);

        tft.setTextColor(TFT_WHITE, bg);
        tft.setTextDatum(TC_DATUM);
        tft.drawString("BRUSH SIZE (1 - 100px)", 120, 32, 2);

        // Live preview circle with Anti-Aliasing
        tft.fillRect(10, 56, 220, 110, TFT_BLACK);
        tft.drawRect(10, 56, 220, 110, 0x3186);
        int prevRadius = max(1, brushSize / 2);
        UI_AA::fillCircle(tft, 120, 111, prevRadius, isEraser ? 0x52AA : currentColor, TFT_BLACK);
        UI_AA::drawCircle(tft, 120, 111, prevRadius + 1, TFT_WHITE, TFT_BLACK);

        char buf[32];
        snprintf(buf, sizeof(buf), "Diameter: %d px", brushSize);
        tft.setTextColor(TFT_CYAN, bg);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(buf, 120, 178, 2);

        // Slider Bar
        UI_AA::drawRoundRect(tft, 16, 198, 208, 16, 4, TFT_WHITE, bg);
        int fillW = ((brushSize - 1) * 204) / 99;
        tft.fillRect(18, 200, fillW, 12, TFT_CYAN);
        tft.fillRect(18 + fillW, 200, 204 - fillW, 12, 0x18E3);

        // Presets: 2px, 10px, 25px, 50px, 100px
        const uint8_t szPresets[5] = { 2, 10, 25, 50, 100 };
        for (int i = 0; i < 5; i++) {
            int bx = 14 + i * 44;
            uint16_t pCol = (brushSize == szPresets[i]) ? TFT_CYAN : 0x2124;
            UI_AA::fillRoundRect(tft, bx, 224, 40, 24, 3, pCol, bg);
            UI_AA::drawRoundRect(tft, bx, 224, 40, 24, 3, TFT_WHITE, bg);
            tft.setTextColor((brushSize == szPresets[i]) ? TFT_BLACK : TFT_WHITE, pCol);
            char bsz[8];
            snprintf(bsz, sizeof(bsz), "%d", szPresets[i]);
            tft.drawString(bsz, bx + 20, 236, 2);
        }

        // Done Button
        UI_AA::fillRoundRect(tft, 16, 258, 208, 36, 5, TFT_GREEN, bg);
        UI_AA::drawRoundRect(tft, 16, 258, 208, 36, 5, TFT_WHITE, bg);
        tft.setTextColor(TFT_BLACK, TFT_GREEN);
        tft.drawString("DONE", 120, 276, 4);
    }

public:
    AppDrawingPad() {
        canvasBuf = (uint16_t*)ps_malloc(240 * 320 * sizeof(uint16_t));
        if (canvasBuf) {
            memset(canvasBuf, 0, 240 * 320 * sizeof(uint16_t));
        }
    }

    const char* getName() const override { return "Draw Pad"; }
    const char* getDescription() const override { return "Solid brush & anti-aliased UI"; }
    uint16_t getThemeColor() const override { return TFT_GREEN; }

    void onEnter(TFT_eSPI& tft) override {
        colorPickerOpen = false;
        sizePickerOpen = false;
        lastX = -1;
        lastY = -1;

        drawToolbar(tft);
        if (canvasBuf && hasDrawnBefore) {
            redrawCanvas(tft);
        } else {
            clearCanvas(tft);
        }
        drawBottomBar(tft);
    }

    void onUpdate(TFT_eSPI& tft, const TouchPoint& touch, unsigned long dt) override {
        if (!touch.pressed || touch.x == 0 || touch.y == 0) {
            if (millis() - lastContactTime > STROKE_HOLD_MS) {
                lastX = -1;
                lastY = -1;
            }
            return;
        }

        lastContactTime = millis();
        uint16_t tx = touch.x;
        uint16_t ty = touch.y;

        // ==========================================
        // 1. COLOR PICKER MODAL INTERACTION
        // ==========================================
        if (colorPickerOpen) {
            if (tx >= 10 && tx <= 230 && ty >= 54 && ty <= 82) {
                currentHue = ((tx - 10) * 255) / 220;
                drawColorPickerModal(tft);
                return;
            }

            if (tx >= 10 && tx <= 230 && ty >= 92 && ty <= 184) {
                currentSat = ((tx - 10) * 255) / 220;
                currentVal = ((184 - ty) * 255) / 92;
                currentColor = hsvToRgb565(currentHue, currentSat, currentVal);
                isEraser = false;
                drawColorPickerModal(tft);
                return;
            }

            if (touch.justPressed && ty >= 194 && ty <= 238 && tx >= 98 && tx <= 230) {
                const uint16_t pal[8] = { TFT_WHITE, TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_BLUE, TFT_MAGENTA };
                for (int i = 0; i < 8; i++) {
                    int px = 100 + (i % 4) * 32;
                    int py = (i < 4) ? 194 : 216;
                    if (tx >= px && tx <= px + 28 && ty >= py && ty <= py + 18) {
                        currentColor = pal[i];
                        isEraser = false;
                        drawColorPickerModal(tft);
                        return;
                    }
                }
            }

            if (touch.justPressed && ((tx >= 16 && tx <= 224 && ty >= 248 && ty <= 286) || (tx >= 210 && ty <= 48))) {
                currentColor = hsvToRgb565(currentHue, currentSat, currentVal);
                isEraser = false;
                colorPickerOpen = false;
                drawToolbar(tft);
                redrawCanvas(tft);
                drawBottomBar(tft);
                return;
            }
            return;
        }

        // ==========================================
        // 2. SIZE PICKER MODAL INTERACTION
        // ==========================================
        if (sizePickerOpen) {
            if (tx >= 16 && tx <= 224 && ty >= 190 && ty <= 220) {
                brushSize = constrain(1 + ((tx - 16) * 99) / 208, 1, 100);
                drawSizePickerModal(tft);
                return;
            }

            if (touch.justPressed && ty >= 224 && ty <= 248) {
                const uint8_t szPresets[5] = { 2, 10, 25, 50, 100 };
                for (int i = 0; i < 5; i++) {
                    int bx = 14 + i * 44;
                    if (tx >= bx && tx <= bx + 40) {
                        brushSize = szPresets[i];
                        drawSizePickerModal(tft);
                        return;
                    }
                }
            }

            if (touch.justPressed && (ty >= 258 && ty <= 294)) {
                sizePickerOpen = false;
                drawToolbar(tft);
                redrawCanvas(tft);
                drawBottomBar(tft);
                return;
            }
            return;
        }

        // ==========================================
        // 3. TOP TOOLBAR INTERACTION (y: 26..54)
        // ==========================================
        if (touch.justPressed && ty >= 26 && ty <= 54) {
            if (tx >= 4 && tx <= 56) {
                isEraser = !isEraser;
                drawToolbar(tft);
                drawBottomBar(tft);
                return;
            }

            if (tx >= 60 && tx <= 114) {
                sizePickerOpen = true;
                drawSizePickerModal(tft);
                return;
            }

            if (tx >= 118 && tx <= 144) {
                if (brushSize > 5) brushSize -= 5;
                else if (brushSize > 1) brushSize--;
                drawToolbar(tft);
                return;
            }

            if (tx >= 148 && tx <= 174) {
                if (brushSize < 95) brushSize += 5;
                else brushSize = 100;
                drawToolbar(tft);
                return;
            }

            if (tx >= 178 && tx <= 236) {
                clearCanvas(tft);
                return;
            }
        }

        // ==========================================
        // 4. BOTTOM BAR INTERACTION (y: 286..320)
        // ==========================================
        if (touch.justPressed && ty >= 286 && ty <= 320) {
            if ((tx >= 4 && tx <= 36) || (tx >= 178 && tx <= 238)) {
                colorPickerOpen = true;
                drawColorPickerModal(tft);
                return;
            }

            const uint16_t quickPal[6] = { TFT_WHITE, TFT_RED, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_BLUE };
            for (int i = 0; i < 6; i++) {
                int cx = 46 + i * 22;
                if (tx >= cx - 10 && tx <= cx + 10) {
                    currentColor = quickPal[i];
                    isEraser = false;
                    drawToolbar(tft);
                    drawBottomBar(tft);
                    return;
                }
            }
        }

        // ==========================================
        // 5. CANVAS DRAWING (CRISP SOLID STROKES, CLEAN INTERSECTIONS)
        // ==========================================
        if (tx >= CANVAS_LEFT && tx <= CANVAS_RIGHT && ty >= CANVAS_TOP && ty <= CANVAS_BOTTOM) {
            uint16_t drawCol = isEraser ? TFT_BLACK : currentColor;
            int radius = max(1, (int)brushSize / 2);

            if (lastX >= 0 && lastY >= 0) {
                int dx = (int)tx - (int)lastX;
                int dy = (int)ty - (int)lastY;
                int distSq = dx * dx + dy * dy;

                // Smart redraw: ignore redundant sub-pixel jitter (< 1.5px) to prevent clumping
                if (distSq < 2) {
                    return;
                }

                if (distSq < 260 * 260) {
                    // Smooth coordinates to remove high-frequency ADC noise
                    int smoothX = (tx * 3 + lastX) / 4;
                    int smoothY = (ty * 3 + lastY) / 4;

                    drawStrokeSegment(tft, lastX, lastY, smoothX, smoothY, radius, drawCol);
                    lastX = smoothX;
                    lastY = smoothY;
                    hasDrawnBefore = true;
                } else {
                    lastX = tx;
                    lastY = ty;
                }
            } else {
                // First point of stroke (pen down)
                drawSolidCircle(tft, tx, ty, radius, drawCol);
                lastX = tx;
                lastY = ty;
                hasDrawnBefore = true;
            }
        } else {
            lastX = -1;
            lastY = -1;
        }
    }

    void onExit(TFT_eSPI& tft) override {
        colorPickerOpen = false;
        sizePickerOpen = false;
        lastX = -1;
        lastY = -1;
    }
};
