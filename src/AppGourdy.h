#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "pins.h"
#include "UI_AA.h"

class AppGourdy {
public:
    enum State {
        STATE_TITLE,
        STATE_PLAYING,
        STATE_FLOOR_CLEAR,
        STATE_GAME_OVER
    };

private:
    State state = STATE_TITLE;

    // Arena geometry (Portrait 240 x 320)
    static const int ARENA_LEFT = 6;
    static const int ARENA_RIGHT = 234;
    static const int ARENA_TOP = 52;
    static const int ARENA_BOTTOM = 296;

    // Gourdy Player
    float gx = 120.0f;
    float gy = 250.0f;
    float prevGx = 120.0f;
    float prevGy = 250.0f;
    float targetX = 120.0f;
    float targetY = 250.0f;
    float facingAngle = -1.57f; // Radians, facing up initially
    float walkAnimPhase = 0.0f;
    bool isMoving = false;

    int lives = 3;
    float stamina = 100.0f;
    const float MAX_STAMINA = 100.0f;
    unsigned long sugarRushUntil = 0;
    unsigned long invulnerableUntil = 0;

    // Game Progression
    int floorLevel = 1;
    uint32_t score = 0;
    uint32_t highScore = 0;
    int ichorCollected = 0;
    int ichorTotal = 4;
    float machineProgress = 0.0f; // 0.0 .. 100.0%
    bool elevatorUnlocked = false;

    // Pillars / Obstacles for kiting (Center & Flanks)
    struct Obstacle {
        int x, y, w, h;
    };
    static const int NUM_OBSTACLES = 4;
    const Obstacle obstacles[NUM_OBSTACLES] = {
        { 38,  110, 24, 24 },
        { 178, 110, 24, 24 },
        { 54,  200, 28, 20 },
        { 158, 200, 28, 20 }
    };

    // Extraction Machine
    const int MACH_X = 120;
    const int MACH_Y = 135;
    const int MACH_W = 34;
    const int MACH_H = 26;

    // Elevator Door (Top Center)
    const int ELEV_X = 120;
    const int ELEV_Y = 56;
    const int ELEV_W = 44;
    const int ELEV_H = 14;

    // Ichor Drops (Glowing Purple Ink)
    struct IchorDrop {
        float x, y;
        bool active;
    };
    static const int MAX_ICHOR = 8;
    IchorDrop ichorList[MAX_ICHOR];

    // Candy Powerup
    struct CandyItem {
        float x, y;
        bool active;
    } candy;

    // Twisted Enemies (Dandy's World Monsters)
    struct TwistedEnemy {
        float x, y;
        float prevX, prevY;
        float patrolX, patrolY;
        bool chasing;
        float speed;
    };
    static const int MAX_TWISTEDS = 3;
    TwistedEnemy twisteds[MAX_TWISTEDS];
    int activeTwisteds = 1;

    // Sugar Rush touch button (Bottom Right)
    const int BTN_SR_X = 212;
    const int BTN_SR_Y = 274;
    const int BTN_SR_R = 18;
    int candiesStocked = 1;

    // -------------------------------------------------------------------------
    // Reset Floor / Level Setup
    // -------------------------------------------------------------------------
    void setupFloor(int floor) {
        floorLevel = floor;
        gx = 120.0f;
        gy = 255.0f;
        prevGx = gx;
        prevGy = gy;
        targetX = gx;
        targetY = gy;
        machineProgress = 0.0f;
        elevatorUnlocked = false;
        stamina = MAX_STAMINA;

        // Ichor count scales with floor
        ichorTotal = min(MAX_ICHOR, 3 + floorLevel);
        ichorCollected = 0;

        // Preset spawn positions avoiding obstacles
        const float spawnCoords[8][2] = {
            { 30, 80 },  { 210, 80 },
            { 80, 100 }, { 160, 100 },
            { 30, 160 }, { 210, 160 },
            { 85, 230 }, { 155, 230 }
        };

        for (int i = 0; i < MAX_ICHOR; i++) {
            if (i < ichorTotal) {
                ichorList[i].x = spawnCoords[i][0];
                ichorList[i].y = spawnCoords[i][1];
                ichorList[i].active = true;
            } else {
                ichorList[i].active = false;
            }
        }

        // Spawn 1 Candy in arena
        candy.x = (floorLevel % 2 == 0) ? 120 : 35;
        candy.y = (floorLevel % 2 == 0) ? 190 : 250;
        candy.active = true;

        // Configure Twisteds
        activeTwisteds = constrain(1 + (floorLevel - 1) / 2, 1, MAX_TWISTEDS);
        for (int i = 0; i < activeTwisteds; i++) {
            twisteds[i].x = 40 + i * 80;
            twisteds[i].y = 80 + (i % 2) * 50;
            twisteds[i].prevX = twisteds[i].x;
            twisteds[i].prevY = twisteds[i].y;
            twisteds[i].patrolX = twisteds[i].x;
            twisteds[i].patrolY = twisteds[i].y;
            twisteds[i].chasing = false;
            twisteds[i].speed = 1.15f + (floorLevel * 0.12f);
        }
    }

    void resetGame() {
        score = 0;
        lives = 3;
        candiesStocked = 1;
        sugarRushUntil = 0;
        invulnerableUntil = 0;
        setupFloor(1);
    }

    // -------------------------------------------------------------------------
    // Collision Helpers
    // -------------------------------------------------------------------------
    bool checkObstacleCollision(float nx, float ny, float radius) {
        for (int i = 0; i < NUM_OBSTACLES; i++) {
            const Obstacle& ob = obstacles[i];
            if (nx + radius > ob.x && nx - radius < ob.x + ob.w &&
                ny + radius > ob.y && ny - radius < ob.y + ob.h) {
                return true;
            }
        }
        // Machine collision
        if (nx + radius > MACH_X - MACH_W / 2 && nx - radius < MACH_X + MACH_W / 2 &&
            ny + radius > MACH_Y - MACH_H / 2 && ny - radius < MACH_Y + MACH_H / 2) {
            return true;
        }
        return false;
    }

    // -------------------------------------------------------------------------
    // Drawing Helpers (Anti-Aliased Gourdy & World)
    // -------------------------------------------------------------------------
    void drawGourdy(TFT_eSPI& tft, int cx, int cy, float angle, bool walking, bool sugarRush, bool blink) {
        if (blink) return; // Invulnerability blink frame

        const uint16_t bg = 0x0842; // Arena dark floor

        // 1. Sugar Rush Golden Halo
        if (sugarRush) {
            UI_AA::drawCircle(tft, cx, cy, 14, TFT_YELLOW, bg);
            UI_AA::drawCircle(tft, cx, cy, 15, TFT_MAGENTA, bg);
        }

        // 2. Green Vine Legs (animated)
        int legOff1 = walking ? (int)(sinf(walkAnimPhase) * 3.0f) : 0;
        int legOff2 = walking ? (int)(-sinf(walkAnimPhase) * 3.0f) : 0;
        tft.fillRect(cx - 5, cy + 9 + legOff1, 3, 4, 0x24C5); // Green vines
        tft.fillRect(cx + 2, cy + 9 + legOff2, 3, 4, 0x24C5);

        // 3. Main Jack-o'-Lantern Pumpkin Head (Composite anti-aliased lobes)
        // Left & Right pumpkin lobes
        UI_AA::fillCircle(tft, cx - 4, cy, 7, 0xFA40, bg); // Darker orange
        UI_AA::fillCircle(tft, cx + 4, cy, 7, 0xFA40, bg);
        // Center main lobe
        UI_AA::fillCircle(tft, cx, cy, 8, 0xFD20, bg); // Bright pumpkin orange

        // Segment crease lines
        tft.drawFastVLine(cx - 3, cy - 6, 12, 0xD280);
        tft.drawFastVLine(cx + 3, cy - 6, 12, 0xD280);

        // Head seam (horizontal line across forehead, Dandy's World detail)
        tft.drawFastHLine(cx - 6, cy - 2, 13, 0xB200);

        // 4. Brown Stem & Green Curled Leaf
        tft.fillRect(cx - 1, cy - 11, 3, 4, 0x69A1); // Woody brown
        tft.fillRect(cx + 2, cy - 11, 4, 2, 0x24C5); // Curled leaf
        tft.drawPixel(cx + 4, cy - 12, 0x47E0);

        // 5. Carved Hollow Eyes with Dynamic Pupils
        // Carved black eye sockets
        tft.fillRect(cx - 5, cy - 4, 3, 3, TFT_BLACK);
        tft.fillRect(cx + 2, cy - 4, 3, 3, TFT_BLACK);

        // White pupils tracking facing direction
        int lookDx = (int)constrain(cosf(angle) * 1.5f, -1.0f, 1.0f);
        int lookDy = (int)constrain(sinf(angle) * 1.5f, -1.0f, 1.0f);
        tft.drawPixel(cx - 4 + lookDx, cy - 3 + lookDy, TFT_WHITE);
        tft.drawPixel(cx + 3 + lookDx, cy - 3 + lookDy, TFT_WHITE);

        // 6. Mischievous Fanged Grin
        tft.drawFastHLine(cx - 4, cy + 3, 9, TFT_BLACK);
        tft.drawPixel(cx - 3, cy + 4, TFT_BLACK);
        tft.drawPixel(cx + 3, cy + 4, TFT_BLACK);
        tft.drawPixel(cx, cy + 4, TFT_BLACK);
        // Little white fangs
        tft.drawPixel(cx - 2, cy + 3, TFT_WHITE);
        tft.drawPixel(cx + 2, cy + 3, TFT_WHITE);
    }

    void drawTwisted(TFT_eSPI& tft, int cx, int cy, bool isChasing) {
        const uint16_t bg = 0x0842;
        // Inky shadow body
        UI_AA::fillCircle(tft, cx, cy, 9, 0x10A2, bg); // Dark inky indigo
        UI_AA::drawCircle(tft, cx, cy, 10, isChasing ? TFT_RED : 0x3186, bg);

        // Inky dripping tendrils
        tft.drawFastVLine(cx - 4, cy + 7, 4, 0x10A2);
        tft.drawFastVLine(cx, cy + 8, 5, 0x10A2);
        tft.drawFastVLine(cx + 4, cy + 7, 4, 0x10A2);

        // Piercing red eyes
        tft.fillRect(cx - 4, cy - 3, 3, 3, TFT_RED);
        tft.fillRect(cx + 2, cy - 3, 3, 3, TFT_RED);
        tft.drawPixel(cx - 3, cy - 2, TFT_WHITE);
        tft.drawPixel(cx + 3, cy - 2, TFT_WHITE);

        // Alert exclamation badge if chasing
        if (isChasing) {
            UI_AA::fillRoundRect(tft, cx - 4, cy - 18, 9, 11, 2, TFT_RED, bg);
            tft.setTextColor(TFT_WHITE, TFT_RED);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("!", cx, cy - 13, 2);
        }
    }

    void drawIchor(TFT_eSPI& tft, int cx, int cy) {
        const uint16_t bg = 0x0842;
        // Floating purple Ichor droplet with white glint
        int floatY = cy + (int)(sinf((millis() + cx * 10) / 250.0f) * 2.0f);
        UI_AA::fillCircle(tft, cx, floatY, 5, 0xC81F, bg); // Neon Purple
        UI_AA::drawCircle(tft, cx, floatY, 5, TFT_WHITE, bg);
        tft.drawPixel(cx - 1, floatY - 2, TFT_WHITE);
    }

    void drawCandy(TFT_eSPI& tft, int cx, int cy) {
        const uint16_t bg = 0x0842;
        // Wrapped Candy: golden center with twist ends
        UI_AA::fillCircle(tft, cx, cy, 4, TFT_YELLOW, bg);
        tft.fillRect(cx - 7, cy - 2, 3, 4, TFT_ORANGE);
        tft.fillRect(cx + 5, cy - 2, 3, 4, TFT_ORANGE);
    }

    void drawWorldStatic(TFT_eSPI& tft) {
        // Clear arena
        tft.fillRect(0, 50, 240, 270, 0x0842); // Dark floor

        // Grid lines for Gardenview floor tiles
        for (int y = ARENA_TOP + 20; y < ARENA_BOTTOM; y += 30) {
            tft.drawFastHLine(ARENA_LEFT, y, ARENA_RIGHT - ARENA_LEFT, 0x1084);
        }
        for (int x = ARENA_LEFT + 25; x < ARENA_RIGHT; x += 30) {
            tft.drawFastVLine(x, ARENA_TOP, ARENA_BOTTOM - ARENA_TOP, 0x1084);
        }

        // Arena outer border
        UI_AA::drawRoundRect(tft, ARENA_LEFT - 2, ARENA_TOP - 2, 
                             (ARENA_RIGHT - ARENA_LEFT) + 4, 
                             (ARENA_BOTTOM - ARENA_TOP) + 4, 6, 0x31E8, 0x0842);

        // Pillars / Obstacles
        for (int i = 0; i < NUM_OBSTACLES; i++) {
            const Obstacle& ob = obstacles[i];
            UI_AA::fillRoundRect(tft, ob.x, ob.y, ob.w, ob.h, 4, 0x2125, 0x0842);
            UI_AA::drawRoundRect(tft, ob.x, ob.y, ob.w, ob.h, 4, 0x4228, 0x0842);
        }

        // Extraction Machine
        UI_AA::fillRoundRect(tft, MACH_X - MACH_W / 2, MACH_Y - MACH_H / 2, MACH_W, MACH_H, 4, 0x18E3, 0x0842);
        UI_AA::drawRoundRect(tft, MACH_X - MACH_W / 2, MACH_Y - MACH_H / 2, MACH_W, MACH_H, 4, TFT_CYAN, 0x0842);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, 0x18E3);
        tft.drawString("EXT", MACH_X, MACH_Y - 4, 1);

        // Elevator Door (Top)
        uint16_t elevCol = elevatorUnlocked ? TFT_GREEN : 0x9000;
        UI_AA::fillRoundRect(tft, ELEV_X - ELEV_W / 2, ELEV_Y, ELEV_W, ELEV_H, 3, elevCol, 0x0842);
        UI_AA::drawRoundRect(tft, ELEV_X - ELEV_W / 2, ELEV_Y, ELEV_W, ELEV_H, 3, TFT_WHITE, 0x0842);
        tft.setTextColor(TFT_WHITE, elevCol);
        tft.drawString(elevatorUnlocked ? "OPEN" : "EXIT", ELEV_X, ELEV_Y + 7, 1);
    }

    void drawHUD(TFT_eSPI& tft) {
        // Header bar (y: 26..50)
        tft.fillRect(0, 26, 240, 24, 0x1082);
        tft.drawFastHLine(0, 50, 240, 0x2965);

        // < ARC Button (x: 4..48, y: 28..47)
        UI_AA::fillRoundRect(tft, 4, 28, 44, 20, 3, 0x2124, 0x1082);
        UI_AA::drawRoundRect(tft, 4, 28, 44, 20, 3, TFT_CYAN, 0x1082);
        tft.setTextColor(TFT_CYAN, 0x2124);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("< ARC", 26, 38, 1);

        // Floor level
        char buf[32];
        snprintf(buf, sizeof(buf), "F%d", floorLevel);
        tft.setTextColor(TFT_YELLOW, 0x1082);
        tft.setTextDatum(TL_DATUM);
        tft.drawString(buf, 54, 30, 2);

        // Ichor Count
        snprintf(buf, sizeof(buf), "Ichor: %d/%d", ichorCollected, ichorTotal);
        tft.setTextColor(0xD81F, 0x1082);
        tft.drawString(buf, 88, 30, 2);

        // Gourdy Lives (Jack-o'-lantern hearts)
        for (int i = 0; i < 3; i++) {
            int hx = 188 + i * 16;
            if (i < lives) {
                UI_AA::fillCircle(tft, hx, 38, 5, 0xFD20, 0x1082); // Orange pumpkin
                tft.drawPixel(hx, 34, 0x24C5); // Green stem
            } else {
                UI_AA::drawCircle(tft, hx, 38, 5, 0x52AA, 0x1082);
            }
        }

        // Sugar Rush Touch Button (Bottom Right)
        bool hasBoost = (candiesStocked > 0) || (millis() < sugarRushUntil);
        uint16_t btnCol = (millis() < sugarRushUntil) ? TFT_MAGENTA : (hasBoost ? TFT_ORANGE : 0x3186);
        UI_AA::fillCircle(tft, BTN_SR_X, BTN_SR_Y, BTN_SR_R, btnCol, 0x0842);
        UI_AA::drawCircle(tft, BTN_SR_X, BTN_SR_Y, BTN_SR_R, TFT_WHITE, 0x0842);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, btnCol);
        tft.drawString(millis() < sugarRushUntil ? "RUSH!" : "CANDY", BTN_SR_X, BTN_SR_Y, 1);
    }

    void drawMachineProgress(TFT_eSPI& tft) {
        // Progress bar below machine
        int bx = MACH_X - MACH_W / 2;
        int by = MACH_Y + MACH_H / 2 + 3;
        tft.drawRect(bx, by, MACH_W, 5, TFT_WHITE);
        int fillW = (int)((machineProgress * (MACH_W - 2)) / 100.0f);
        tft.fillRect(bx + 1, by + 1, fillW, 3, TFT_GREEN);
        tft.fillRect(bx + 1 + fillW, by + 1, (MACH_W - 2) - fillW, 3, TFT_BLACK);
    }

public:
    AppGourdy() {}

    void onEnter(TFT_eSPI& tft) {
        state = STATE_TITLE;
        highScore = max(highScore, score);
        drawTitleScreen(tft);
    }

    void drawTitleScreen(TFT_eSPI& tft) {
        tft.fillScreen(TFT_BLACK);
        // Header
        tft.fillRect(0, 26, 240, 24, 0x1082);
        UI_AA::fillRoundRect(tft, 4, 28, 44, 20, 3, 0x2124, 0x1082);
        UI_AA::drawRoundRect(tft, 4, 28, 44, 20, 3, TFT_CYAN, 0x1082);
        tft.setTextColor(TFT_CYAN, 0x2124);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("< ARC", 26, 38, 1);

        tft.setTextColor(TFT_CYAN, 0x1082);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("DANDY'S WORLD", 140, 38, 2);

        // Big Gourdy Illustration
        drawGourdy(tft, 120, 110, -1.57f, false, true, false);

        // Title Cards
        tft.setTextColor(TFT_ORANGE, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("GOURDY'S ESCAPE", 120, 145, 4);

        tft.setTextColor(0xD81F, TFT_BLACK);
        tft.drawString("Collect Ichor & Dodge Twisteds!", 120, 172, 2);

        tft.setTextColor(0x94B2, TFT_BLACK);
        tft.drawString("Touch to steer Gourdy Holloway", 120, 195, 2);
        tft.drawString("Extract the machine to open Elevator", 120, 212, 1);

        char hbuf[32];
        snprintf(hbuf, sizeof(hbuf), "High Score: %u", (unsigned)highScore);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawString(hbuf, 120, 235, 2);

        // Start Button
        UI_AA::fillRoundRect(tft, 30, 258, 180, 42, 6, TFT_GREEN, TFT_BLACK);
        UI_AA::drawRoundRect(tft, 30, 258, 180, 42, 6, TFT_WHITE, TFT_BLACK);
        tft.setTextColor(TFT_BLACK, TFT_GREEN);
        tft.drawString("TAP TO PLAY", 120, 279, 4);
    }

    void onUpdate(TFT_eSPI& tft, const TouchPoint& touch, unsigned long dt) {
        // Title Screen Interaction
        if (state == STATE_TITLE) {
            if (touch.justPressed && touch.y > 50) {
                resetGame();
                state = STATE_PLAYING;
                drawWorldStatic(tft);
                drawHUD(tft);
            }
            return;
        }

        // Game Over Interaction
        if (state == STATE_GAME_OVER) {
            if (touch.justPressed && touch.y > 50) {
                resetGame();
                state = STATE_PLAYING;
                drawWorldStatic(tft);
                drawHUD(tft);
            }
            return;
        }

        // Floor Clear Cutscene
        if (state == STATE_FLOOR_CLEAR) {
            static unsigned long clearTimer = 0;
            if (clearTimer == 0) clearTimer = millis();
            if (millis() - clearTimer > 1500) {
                clearTimer = 0;
                setupFloor(floorLevel + 1);
                state = STATE_PLAYING;
                drawWorldStatic(tft);
                drawHUD(tft);
            }
            return;
        }

        // =====================================================================
        // PLAYING STATE
        // =====================================================================
        // 1. Touch Input: Sugar Rush Button or Movement Steer
        if (touch.pressed) {
            // Check Sugar Rush Candy button (x: 190..235, y: 250..300)
            int bdx = (int)touch.x - BTN_SR_X;
            int bdy = (int)touch.y - BTN_SR_Y;
            if (bdx * bdx + bdy * bdy <= (BTN_SR_R + 6) * (BTN_SR_R + 6)) {
                if (touch.justPressed && candiesStocked > 0 && millis() > sugarRushUntil) {
                    candiesStocked--;
                    sugarRushUntil = millis() + 4500; // 4.5 seconds Sugar Rush!
                    drawHUD(tft);
                }
            } else if (touch.y >= ARENA_TOP && touch.y <= ARENA_BOTTOM) {
                targetX = constrain((float)touch.x, (float)ARENA_LEFT + 12, (float)ARENA_RIGHT - 12);
                targetY = constrain((float)touch.y, (float)ARENA_TOP + 12, (float)ARENA_BOTTOM - 12);
            }
        }

        // 2. Gourdy Physics & Movement
        bool hasRush = (millis() < sugarRushUntil);
        float currentSpeed = hasRush ? 3.6f : 2.4f;

        float dx = targetX - gx;
        float dy = targetY - gy;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist > 3.0f) {
            isMoving = true;
            facingAngle = atan2f(dy, dx);
            walkAnimPhase += 0.35f;

            float stepX = (dx / dist) * min(dist, currentSpeed);
            float stepY = (dy / dist) * min(dist, currentSpeed);

            float nextX = gx + stepX;
            float nextY = gy + stepY;

            // Obstacle & Arena Boundary Collision
            if (!checkObstacleCollision(nextX, gy, 8.0f)) {
                gx = constrain(nextX, (float)ARENA_LEFT + 8, (float)ARENA_RIGHT - 8);
            }
            if (!checkObstacleCollision(gx, nextY, 8.0f)) {
                gy = constrain(nextY, (float)ARENA_TOP + 8, (float)ARENA_BOTTOM - 8);
            }

            // Stamina drain
            if (!hasRush) {
                stamina = max(0.0f, stamina - 0.2f);
            }
        } else {
            isMoving = false;
            // Stamina recovery when resting
            stamina = min(MAX_STAMINA, stamina + 0.4f);
        }

        // 3. Ichor Collection
        for (int i = 0; i < ichorTotal; i++) {
            if (ichorList[i].active) {
                float idx = gx - ichorList[i].x;
                float idy = gy - ichorList[i].y;
                if (idx * idx + idy * idy < 14.0f * 14.0f) {
                    ichorList[i].active = false;
                    ichorCollected++;
                    score += 50 * floorLevel;
                    // Erase Ichor spot cleanly
                    tft.fillRect((int)ichorList[i].x - 8, (int)ichorList[i].y - 8, 16, 16, 0x0842);
                    drawHUD(tft);
                }
            }
        }

        // 4. Candy Collection
        if (candy.active) {
            float cdx = gx - candy.x;
            float cdy = gy - candy.y;
            if (cdx * cdx + cdy * cdy < 14.0f * 14.0f) {
                candy.active = false;
                candiesStocked = min(3, candiesStocked + 1);
                score += 100;
                tft.fillRect((int)candy.x - 10, (int)candy.y - 6, 20, 12, 0x0842);
                drawHUD(tft);
            }
        }

        // 5. Machine Extraction
        float mdx = gx - MACH_X;
        float mdy = gy - MACH_Y;
        if (mdx * mdx + mdy * mdy < 36.0f * 36.0f && machineProgress < 100.0f) {
            machineProgress += 0.65f;
            score += 1;
            drawMachineProgress(tft);
            if (machineProgress >= 100.0f) {
                score += 250 * floorLevel;
            }
        }

        // Check if Elevator should unlock
        if (!elevatorUnlocked && ichorCollected >= ichorTotal && machineProgress >= 100.0f) {
            elevatorUnlocked = true;
            // Redraw elevator green
            UI_AA::fillRoundRect(tft, ELEV_X - ELEV_W / 2, ELEV_Y, ELEV_W, ELEV_H, 3, TFT_GREEN, 0x0842);
            UI_AA::drawRoundRect(tft, ELEV_X - ELEV_W / 2, ELEV_Y, ELEV_W, ELEV_H, 3, TFT_WHITE, 0x0842);
            tft.setTextDatum(MC_DATUM);
            tft.setTextColor(TFT_WHITE, TFT_GREEN);
            tft.drawString("OPEN", ELEV_X, ELEV_Y + 7, 1);
        }

        // 6. Elevator Exit Trigger
        if (elevatorUnlocked && fabsf(gx - ELEV_X) < 18.0f && gy < ELEV_Y + ELEV_H + 8) {
            state = STATE_FLOOR_CLEAR;
            score += 500 * floorLevel;
            if (score > highScore) highScore = score;

            // Floor clear banner
            UI_AA::fillRoundRect(tft, 25, 140, 190, 60, 6, 0x0400, 0x0842);
            UI_AA::drawRoundRect(tft, 25, 140, 190, 60, 6, TFT_GREEN, 0x0842);
            tft.setTextDatum(MC_DATUM);
            tft.setTextColor(TFT_GREEN, 0x0400);
            tft.drawString("FLOOR CLEARED!", 120, 160, 4);
            tft.setTextColor(TFT_WHITE, 0x0400);
            tft.drawString("Ascending to Next Floor...", 120, 185, 2);
            return;
        }

        // 7. Twisted Enemy AI & Movement
        for (int i = 0; i < activeTwisteds; i++) {
            TwistedEnemy& tw = twisteds[i];
            float tdx = gx - tw.x;
            float tdy = gy - tw.y;
            float tDist = sqrtf(tdx * tdx + tdy * tdy);

            // Detection radius: 75px (Gourdy has low stealth!)
            tw.chasing = (tDist < 75.0f);

            if (tw.chasing) {
                tw.x += (tdx / tDist) * tw.speed;
                tw.y += (tdy / tDist) * tw.speed;
            } else {
                // Gentle patrol drift
                float pdx = tw.patrolX - tw.x;
                float pdy = tw.patrolY - tw.y;
                float pDist = sqrtf(pdx * pdx + pdy * pdy);
                if (pDist < 4.0f) {
                    tw.patrolX = random(ARENA_LEFT + 20, ARENA_RIGHT - 20);
                    tw.patrolY = random(ARENA_TOP + 30, ARENA_BOTTOM - 30);
                } else {
                    tw.x += (pdx / pDist) * (tw.speed * 0.5f);
                    tw.y += (pdy / pDist) * (tw.speed * 0.5f);
                }
            }

            // Twisted Collision with Gourdy
            if (tDist < 16.0f && millis() > invulnerableUntil) {
                lives--;
                invulnerableUntil = millis() + 1800; // 1.8s invulnerability
                drawHUD(tft);

                if (lives <= 0) {
                    state = STATE_GAME_OVER;
                    if (score > highScore) highScore = score;

                    UI_AA::fillRoundRect(tft, 25, 140, 190, 68, 6, 0x8000, 0x0842);
                    UI_AA::drawRoundRect(tft, 25, 140, 190, 68, 6, TFT_RED, 0x0842);
                    tft.setTextDatum(MC_DATUM);
                    tft.setTextColor(TFT_RED, 0x8000);
                    tft.drawString("TWISTED CAUGHT YOU!", 120, 160, 2);
                    tft.setTextColor(TFT_WHITE, 0x8000);
                    tft.drawString("Tap to try again", 120, 185, 2);
                    return;
                }
            }
        }

        // =====================================================================
        // DIFFERENTIAL 60FPS RENDERING
        // =====================================================================
        // Erase previous Gourdy
        tft.fillRect((int)prevGx - 12, (int)prevGy - 14, 25, 28, 0x0842);

        // Erase previous Twisteds
        for (int i = 0; i < activeTwisteds; i++) {
            tft.fillRect((int)twisteds[i].prevX - 12, (int)twisteds[i].prevY - 20, 25, 36, 0x0842);
        }

        // Draw Ichor drops
        for (int i = 0; i < ichorTotal; i++) {
            if (ichorList[i].active) {
                drawIchor(tft, (int)ichorList[i].x, (int)ichorList[i].y);
            }
        }

        // Draw Candy
        if (candy.active) {
            drawCandy(tft, (int)candy.x, (int)candy.y);
        }

        // Draw Twisteds
        for (int i = 0; i < activeTwisteds; i++) {
            drawTwisted(tft, (int)twisteds[i].x, (int)twisteds[i].y, twisteds[i].chasing);
            twisteds[i].prevX = twisteds[i].x;
            twisteds[i].prevY = twisteds[i].y;
        }

        // Draw Gourdy
        bool blink = (millis() < invulnerableUntil) && ((millis() / 150) % 2 == 0);
        drawGourdy(tft, (int)gx, (int)gy, facingAngle, isMoving, hasRush, blink);
        prevGx = gx;
        prevGy = gy;
    }

    void onExit(TFT_eSPI& tft) {}
};
