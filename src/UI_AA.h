#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <math.h>

class UI_AA {
public:
    // -------------------------------------------------------------------------
    // Anti-Aliased Filled Circle against a known background color
    // -------------------------------------------------------------------------
    static void fillCircle(TFT_eSPI& tft, int cx, int cy, int r, uint16_t color, uint16_t bg) {
        if (r <= 0) return;
        float rInner = max(0.0f, (float)r - 0.75f);
        float rOuter = (float)r + 0.75f;
        float rInnerSq = rInner * rInner;
        float rOuterSq = rOuter * rOuter;
        float span = rOuter - rInner;

        int minY = cy - r - 1;
        int maxY = cy + r + 1;

        for (int y = minY; y <= maxY; y++) {
            float dy = (float)y - cy;
            float dySq = dy * dy;
            if (dySq >= rOuterSq) continue;

            float dxMax = sqrtf(rOuterSq - dySq);
            int minX = (int)floorf(cx - dxMax);
            int maxX = (int)ceilf(cx + dxMax);

            int coreMinX = 999;
            int coreMaxX = -999;
            if (dySq < rInnerSq) {
                float dxCore = sqrtf(rInnerSq - dySq);
                coreMinX = (int)ceilf(cx - dxCore);
                coreMaxX = (int)floorf(cx + dxCore);
            }

            for (int x = minX; x <= maxX; x++) {
                if (x >= coreMinX && x <= coreMaxX) {
                    int len = coreMaxX - x + 1;
                    tft.drawFastHLine(x, y, len, color);
                    x = coreMaxX;
                } else {
                    float dx = (float)x - cx;
                    float d = sqrtf(dx * dx + dySq);
                    if (d < rOuter) {
                        uint8_t alpha = 255;
                        if (d > rInner) {
                            alpha = (uint8_t)constrain(255.0f * (rOuter - d) / span, 0.0f, 255.0f);
                        }
                        if (alpha > 0) {
                            tft.drawPixel(x, y, tft.alphaBlend(alpha, color, bg));
                        }
                    }
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // Anti-Aliased Circle Outline
    // -------------------------------------------------------------------------
    static void drawCircle(TFT_eSPI& tft, int cx, int cy, int r, uint16_t color, uint16_t bg) {
        if (r <= 0) return;
        float rInner = max(0.0f, (float)r - 1.25f);
        float rOuter = (float)r + 1.25f;
        float rPeak = (float)r;

        int minY = cy - r - 2;
        int maxY = cy + r + 2;

        for (int y = minY; y <= maxY; y++) {
            float dy = (float)y - cy;
            float dySq = dy * dy;
            if (dySq >= rOuter * rOuter) continue;

            float dxMax = sqrtf(rOuter * rOuter - dySq);
            int minX = (int)floorf(cx - dxMax);
            int maxX = (int)ceilf(cx + dxMax);

            for (int x = minX; x <= maxX; x++) {
                float dx = (float)x - cx;
                float d = sqrtf(dx * dx + dySq);
                if (d >= rInner && d <= rOuter) {
                    float distFromCenter = fabsf(d - rPeak);
                    uint8_t alpha = (uint8_t)constrain(255.0f * (1.25f - distCenter(d, rPeak)) / 1.25f, 0.0f, 255.0f);
                    if (alpha > 0) {
                        tft.drawPixel(x, y, tft.alphaBlend(alpha, color, bg));
                    }
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // Anti-Aliased Filled Rounded Rectangle
    // -------------------------------------------------------------------------
    static void fillRoundRect(TFT_eSPI& tft, int x, int y, int w, int h, int r, uint16_t color, uint16_t bg) {
        if (w <= 0 || h <= 0) return;
        r = min(r, min(w / 2, h / 2));
        if (r <= 0) {
            tft.fillRect(x, y, w, h, color);
            return;
        }

        // Center rectangular core
        tft.fillRect(x, y + r, w, h - 2 * r, color);

        // Top and bottom middle spans
        if (w > 2 * r) {
            tft.fillRect(x + r, y, w - 2 * r, r, color);
            tft.fillRect(x + r, y + h - r, w - 2 * r, r, color);
        }

        // 4 Anti-Aliased Corners
        float rInner = max(0.0f, (float)r - 0.75f);
        float rOuter = (float)r + 0.75f;
        float rInnerSq = rInner * rInner;
        float rOuterSq = rOuter * rOuter;
        float span = rOuter - rInner;

        int cx[4] = { x + r, x + w - 1 - r, x + r, x + w - 1 - r };
        int cy[4] = { y + r, y + r, y + h - 1 - r, y + h - 1 - r };
        int signX[4] = { -1, 1, -1, 1 };
        int signY[4] = { -1, -1, 1, 1 };

        for (int c = 0; c < 4; c++) {
            for (int dy = 0; dy <= r; dy++) {
                float dySq = (float)(dy * dy);
                if (dySq >= rOuterSq) continue;

                int py = cy[c] + signY[c] * dy;
                for (int dx = 0; dx <= r; dx++) {
                    float dxSq = (float)(dx * dx);
                    float dSq = dxSq + dySq;
                    int px = cx[c] + signX[c] * dx;

                    if (dSq <= rInnerSq) {
                        tft.drawPixel(px, py, color);
                    } else if (dSq < rOuterSq) {
                        float d = sqrtf(dSq);
                        uint8_t alpha = (uint8_t)constrain(255.0f * (rOuter - d) / span, 0.0f, 255.0f);
                        if (alpha > 0) {
                            tft.drawPixel(px, py, tft.alphaBlend(alpha, color, bg));
                        }
                    }
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // Anti-Aliased Rounded Rectangle Outline
    // -------------------------------------------------------------------------
    static void drawRoundRect(TFT_eSPI& tft, int x, int y, int w, int h, int r, uint16_t color, uint16_t bg) {
        if (w <= 0 || h <= 0) return;
        r = min(r, min(w / 2, h / 2));
        if (r <= 0) {
            tft.drawRect(x, y, w, h, color);
            return;
        }

        // Straight borders
        if (w > 2 * r) {
            tft.drawFastHLine(x + r, y, w - 2 * r, color);
            tft.drawFastHLine(x + r, y + h - 1, w - 2 * r, color);
        }
        if (h > 2 * r) {
            tft.drawFastVLine(x, y + r, h - 2 * r, color);
            tft.drawFastVLine(x + w - 1, y + r, h - 2 * r, color);
        }

        // 4 Anti-aliased corner curves
        float rInner = max(0.0f, (float)r - 1.25f);
        float rOuter = (float)r + 1.25f;

        int cx[4] = { x + r, x + w - 1 - r, x + r, x + w - 1 - r };
        int cy[4] = { y + r, y + r, y + h - 1 - r, y + h - 1 - r };
        int signX[4] = { -1, 1, -1, 1 };
        int signY[4] = { -1, -1, 1, 1 };

        for (int c = 0; c < 4; c++) {
            for (int dy = 0; dy <= r + 1; dy++) {
                int py = cy[c] + signY[c] * dy;
                for (int dx = 0; dx <= r + 1; dx++) {
                    float d = sqrtf((float)(dx * dx + dy * dy));
                    if (d >= rInner && d <= rOuter) {
                        int px = cx[c] + signX[c] * dx;
                        float distCenter = fabsf(d - (float)r);
                        uint8_t alpha = (uint8_t)constrain(255.0f * (1.25f - distCenter) / 1.25f, 0.0f, 255.0f);
                        if (alpha > 0) {
                            tft.drawPixel(px, py, tft.alphaBlend(alpha, color, bg));
                        }
                    }
                }
            }
        }
    }

private:
    static float distCenter(float d, float rPeak) {
        return fabsf(d - rPeak);
    }
};
