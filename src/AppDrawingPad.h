#pragma once
#include "App.h"
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

    // In-memory 16-bit framebuffer in 8MB PSRAM for sub-pixel anti-aliased blending & persistence
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
    // ANTI-ALIASED CIRCLE STAMP WITH PSRAM COLOR BLENDING
    // =========================================================================
    void drawSmoothStamp(TFT_eSPI& tft, float cx, float cy, float radius, uint16_t color) {
        if (radius <= 0.75f) {
            int ix = (int)(cx + 0.5f);
            int iy = (int)(cy + 0.5f);
            if (ix >= CANVAS_LEFT && ix <= CANVAS_RIGHT && iy >= CANVAS_TOP && iy <= CANVAS_BOTTOM) {
                if (canvasBuf) canvasBuf[iy * 240 + ix] = color;
                tft.drawPixel(ix, iy, color);
            }
            return;
        }

        // Sub-pixel anti-aliasing feather: 0.75 pixel transition ring
        float rInner = max(0.0f, radius - 0.75f);
        float rOuter = radius + 0.75f;
        float rInnerSq = rInner * rInner;
        float rOuterSq = rOuter * rOuter;
        float rFringeSpan = rOuter - rInner;

        int minPy = max((int)CANVAS_TOP, (int)floorf(cy - rOuter));
        int maxPy = min((int)CANVAS_BOTTOM, (int)ceilf(cy + rOuter));

        for (int py = minPy; py <= maxPy; py++) {
            float dy = (float)py - cy;
            float dySq = dy * dy;
            if (dySq >= rOuterSq) continue;

            float dxMax = sqrtf(rOuterSq - dySq);
            int minPx = max((int)CANVAS_LEFT, (int)floorf(cx - dxMax));
            int maxPx = min((int)CANVAS_RIGHT, (int)ceilf(cx + dxMax));

            int coreMinX = 999;
            int coreMaxX = -999;
            if (dySq < rInnerSq) {
                float dxCore = sqrtf(rInnerSq - dySq);
                coreMinX = max((int)CANVAS_LEFT, (int)ceilf(cx - dxCore));
                coreMaxX = min((int)CANVAS_RIGHT, (int)floorf(cx + dxCore));
            }

            for (int px = minPx; px <= maxPx; px++) {
                if (px >= coreMinX && px <= coreMaxX) {
                    // Fast solid core span
                    int spanLen = coreMaxX - px + 1;
                    if (canvasBuf) {
                        uint16_t* rowPtr = &canvasBuf[py * 240 + px];
                        for (int k = 0; k < spanLen; k++) rowPtr[k] = color;
                    }
                    tft.drawFastHLine(px, py, spanLen, color);
                    px = coreMaxX; // Advance to end of core
                } else {
                    // Anti-aliased outer edge pixel
                    float dx = (float)px - cx;
                    float d = sqrtf(dx * dx + dySq);
                    if (d < rOuter) {
                        uint8_t alpha = 255;
                        if (d > rInner) {
                            alpha = (uint8_t)constrain(255.0f * (rOuter - d) / rFringeSpan, 0.0f, 255.0f);
                        }
                        if (alpha > 0) {
                            uint16_t bg = canvasBuf ? canvasBuf[py * 240 + px] : TFT_BLACK;
                            uint16_t blended = (alpha >= 254) ? color : tft.alphaBlend(alpha, color, bg);
                            if (canvasBuf) canvasBuf[py * 240 + px] = blended;
                            tft.drawPixel(px, py, blended);
                        }
                    }
                }
            }
        }
    }

    // =========================================================================
    // DENSE CONTINUOUS ANTI-ALIASED STROKE INTERPOLATION
    // =========================================================================
    void drawSmoothStroke(TFT_eSPI& tft, float x0, float y0, float x1, float y1, float radius, uint16_t color) {
        float dx = x1 - x0;
        float dy = y1 - y0;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < 0.5f) {
            drawSmoothStamp(tft, x1, y1, radius, color);
            return;
        }

        // Spacing between stamps: radius * 0.35 ensures velvety smooth edges with no scalloping
        float stepDist = max(1.0f, radius * 0.35f);
        int steps = (int)ceilf(dist / stepDist);

        for (int s = 0; s <= steps; s++) {
            float t = (float)s / (float)steps;
            float x = x0 + dx * t;
            float y = y0 + dy * t;
            drawSmoothStamp(tft, x, y, radius, color);
        }
    }

    void drawToolbar(TFT_eSPI& tft) {
        tft.fillRect(0, 26, 240, 28, 0x18E3);
        tft.drawFastHLine(0, 54, 240, 0x4208);

        // 1. Tool Toggle (PEN / ERASE)
        tft.fillRoundRect(4, 28, 52, 24, 3, isEraser ? TFT_RED : 0x037A);
        tft.drawRoundRect(4, 28, 52, 24, 3, TFT_WHITE);
        tft.setTextColor(TFT_WHITE, isEraser ? TFT_RED : 0x037A);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(isEraser ? "ERASE" : "PEN", 30, 40, 2);

        // 2. Brush Size Button
        tft.fillRoundRect(60, 28, 54, 24, 3, 0x2965);
        tft.drawRoundRect(60, 28, 54, 24, 3, TFT_CYAN);
        tft.setTextColor(TFT_CYAN, 0x2965);
        char szBuf[12];
        snprintf(szBuf, sizeof(szBuf), "%dpx", brushSize);
        tft.drawString(szBuf, 87, 40, 2);

        // 3. Quick Size Steppers [-] and [+]
        tft.fillRoundRect(118, 28, 26, 24, 3, 0x2124);
        tft.drawRoundRect(118, 28, 26, 24, 3, TFT_WHITE);
        tft.setTextColor(TFT_WHITE, 0x2124);
        tft.drawString("-", 131, 40, 2);

        tft.fillRoundRect(148, 28, 26, 24, 3, 0x2124);
        tft.drawRoundRect(148, 28, 26, 24, 3, TFT_WHITE);
        tft.setTextColor(TFT_WHITE, 0x2124);
        tft.drawString("+", 161, 40, 2);

        // 4. Clear Canvas Button
        tft.fillRoundRect(178, 28, 58, 24, 3, 0x8800);
        tft.drawRoundRect(178, 28, 58, 24, 3, TFT_WHITE);
        tft.setTextColor(TFT_WHITE, 0x8800);
        tft.drawString("CLEAR", 207, 40, 2);
    }

    void drawBottomBar(TFT_eSPI& tft) {
        tft.fillRect(0, 286, 240, 34, 0x1082);
        tft.drawFastHLine(0, 285, 240, 0x3186);

        // IbisPaint Color Wheel Swatch
        uint16_t activeCol = isEraser ? TFT_BLACK : currentColor;
        tft.fillCircle(18, 302, 12, activeCol);
        tft.drawCircle(18, 302, 13, TFT_WHITE);
        tft.drawCircle(18, 302, 14, TFT_CYAN);

        // Quick Swatches (White, Red, Yellow, Green, Cyan, Blue)
        const uint16_t quickPal[6] = { TFT_WHITE, TFT_RED, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_BLUE };
        for (int i = 0; i < 6; i++) {
            int cx = 46 + i * 22;
            tft.fillCircle(cx, 302, 8, quickPal[i]);
            tft.drawCircle(cx, 302, 9, 0x52AA);
        }

        // Palette Modal Prompt Button
        tft.fillRoundRect(180, 290, 56, 24, 3, 0x3186);
        tft.drawRoundRect(180, 290, 56, 24, 3, TFT_YELLOW);
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
    // IBISPAINT STYLE COLOR PICKER MODAL
    // =========================================================================
    void drawColorPickerModal(TFT_eSPI& tft) {
        tft.fillRect(0, 26, 240, 294, 0x1082);
        tft.drawRect(0, 26, 240, 294, TFT_CYAN);

        tft.setTextColor(TFT_WHITE, 0x1082);
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
        tft.drawCircle(satX, valY, 4, TFT_WHITE);
        tft.drawCircle(satX, valY, 5, TFT_BLACK);

        // 3. Color Preview & Hex Swatch
        uint16_t previewCol = hsvToRgb565(currentHue, currentSat, currentVal);
        tft.fillRoundRect(10, 194, 80, 40, 4, previewCol);
        tft.drawRoundRect(10, 194, 80, 40, 4, TFT_WHITE);

        // 4. Preset Swatches (x: 98..230, y: 194..234)
        const uint16_t pal[8] = { TFT_WHITE, TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_BLUE, TFT_MAGENTA };
        for (int i = 0; i < 8; i++) {
            int px = 100 + (i % 4) * 32;
            int py = (i < 4) ? 194 : 216;
            tft.fillRoundRect(px, py, 28, 18, 3, pal[i]);
            tft.drawRoundRect(px, py, 28, 18, 3, (previewCol == pal[i]) ? TFT_WHITE : 0x4208);
        }

        // 5. Apply Button (x: 16..224, y: 248..286)
        tft.fillRoundRect(16, 248, 208, 38, 5, TFT_GREEN);
        tft.drawRoundRect(16, 248, 208, 38, 5, TFT_WHITE);
        tft.setTextColor(TFT_BLACK, TFT_GREEN);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("APPLY COLOR", 120, 267, 4);

        // Close 'X'
        tft.setTextColor(TFT_RED, 0x1082);
        tft.drawString("X", 224, 38, 4);
    }

    // =========================================================================
    // BRUSH SIZE PICKER MODAL
    // =========================================================================
    void drawSizePickerModal(TFT_eSPI& tft) {
        tft.fillRect(0, 26, 240, 294, 0x1082);
        tft.drawRect(0, 26, 240, 294, TFT_CYAN);

        tft.setTextColor(TFT_WHITE, 0x1082);
        tft.setTextDatum(TC_DATUM);
        tft.drawString("BRUSH SIZE (1 - 100px)", 120, 32, 2);

        // Live preview circle
        tft.fillRect(10, 56, 220, 110, TFT_BLACK);
        tft.drawRect(10, 56, 220, 110, 0x3186);
        tft.fillCircle(120, 111, max(1, brushSize / 2), isEraser ? 0x52AA : currentColor);
        tft.drawCircle(120, 111, max(1, brushSize / 2), TFT_WHITE);

        char buf[32];
        snprintf(buf, sizeof(buf), "Diameter: %d px", brushSize);
        tft.setTextColor(TFT_CYAN, 0x1082);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(buf, 120, 178, 2);

        // Slider Bar
        tft.drawRoundRect(16, 198, 208, 16, 4, TFT_WHITE);
        int fillW = ((brushSize - 1) * 204) / 99;
        tft.fillRect(18, 200, fillW, 12, TFT_CYAN);
        tft.fillRect(18 + fillW, 200, 204 - fillW, 12, 0x18E3);

        // Presets: 2px, 10px, 25px, 50px, 100px
        const uint8_t szPresets[5] = { 2, 10, 25, 50, 100 };
        for (int i = 0; i < 5; i++) {
            int bx = 14 + i * 44;
            tft.fillRoundRect(bx, 224, 40, 24, 3, (brushSize == szPresets[i]) ? TFT_CYAN : 0x2124);
            tft.drawRoundRect(bx, 224, 40, 24, 3, TFT_WHITE);
            tft.setTextColor((brushSize == szPresets[i]) ? TFT_BLACK : TFT_WHITE, (brushSize == szPresets[i]) ? TFT_CYAN : 0x2124);
            char bsz[8];
            snprintf(bsz, sizeof(bsz), "%d", szPresets[i]);
            tft.drawString(bsz, bx + 20, 236, 2);
        }

        // Done Button
        tft.fillRoundRect(16, 258, 208, 36, 5, TFT_GREEN);
        tft.drawRoundRect(16, 258, 208, 36, 5, TFT_WHITE);
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
    const char* getDescription() const override { return "Anti-aliased brush & IbisPaint palette"; }
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
        // 5. ANTI-ALIASED CANVAS DRAWING (1 - 100px)
        // ==========================================
        if (tx >= CANVAS_LEFT && tx <= CANVAS_RIGHT && ty >= CANVAS_TOP && ty <= CANVAS_BOTTOM) {
            uint16_t drawCol = isEraser ? TFT_BLACK : currentColor;
            hasDrawnBefore = true;
            float r = max(0.5f, brushSize / 2.0f);

            int16_t smoothX = tx;
            int16_t smoothY = ty;

            if (lastX >= 0 && lastY >= 0) {
                // Low-pass EMA smoothing: 75% new position, 25% previous
                smoothX = (tx * 3 + lastX) / 4;
                smoothY = (ty * 3 + lastY) / 4;

                int dx = (int)smoothX - (int)lastX;
                int dy = (int)smoothY - (int)lastY;
                int dist = max(abs(dx), abs(dy));

                if (dist < 260) {
                    drawSmoothStroke(tft, (float)lastX, (float)lastY, (float)smoothX, (float)smoothY, r, drawCol);
                }
            } else {
                drawSmoothStamp(tft, (float)smoothX, (float)smoothY, r, drawCol);
            }

            lastX = smoothX;
            lastY = smoothY;
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
