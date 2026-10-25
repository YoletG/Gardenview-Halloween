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
        STATE_DANDY_SHOP,
        STATE_FLOOR_CLEAR,
        STATE_GAME_OVER
    };

    enum TwistedType {
        TWISTED_DANDY = 0,   // Corrupted pumpkin-petal flower, wide detection
        TWISTED_POPPY,       // Corrupted bubble rabbit, erratic & fast
        TWISTED_BOXTEN       // Corrupted music box, charges in burst sprints
    };

    enum TwistedAI {
        AI_PATROL = 0,
        AI_INVESTIGATE,      // Noise heard! Yellow '?'
        AI_CHASE,            // Hunting Gourdy! Red '!'
        AI_STUNNED           // Blown away by flashlight! Spinning '*'
    };

private:
    State state = STATE_TITLE;

    // Arena geometry (Portrait 240 x 320)
    static const int ARENA_LEFT = 6;
    static const int ARENA_RIGHT = 234;
    static const int ARENA_TOP = 52;
    static const int ARENA_BOTTOM = 282;

    // Gourdy Player
    float gx = 120.0f;
    float gy = 245.0f;
    float prevGx = 120.0f;
    float prevGy = 245.0f;
    float targetX = 120.0f;
    float targetY = 245.0f;
    float facingAngle = -1.57f;
    float walkAnimPhase = 0.0f;
    bool isMoving = false;

    int lives = 3;
    float stamina = 100.0f;
    const float MAX_STAMINA = 100.0f;
    unsigned long sugarRushUntil = 0;
    unsigned long invulnerableUntil = 0;

    // Inventory & Abilities
    int candiesStocked = 1;       // Candy Corn for Sugar Rush
    int flashlightCharges = 2;    // Flashlight for Stun
    unsigned long flashlightCooldown = 0;
    unsigned long stunFlashVisualUntil = 0;

    // Currency & Scoring
    int ichorWallet = 0;          // Total research currency saved across floors
    uint32_t score = 0;
    uint32_t highScore = 0;
    int floorLevel = 1;

    // Pillars / Obstacles for kiting (Center & Flanks)
    struct Obstacle {
        int x, y, w, h;
    };
    static const int NUM_OBSTACLES = 4;
    const Obstacle obstacles[NUM_OBSTACLES] = {
        { 34,  105, 24, 24 },
        { 182, 105, 24, 24 },
        { 48,  195, 28, 20 },
        { 164, 195, 28, 20 }
    };

    // Elevator Door (Top Center)
    const int ELEV_X = 120;
    const int ELEV_Y = 56;
    const int ELEV_W = 46;
    const int ELEV_H = 14;
    bool elevatorUnlocked = false;

    // =========================================================================
    // 1. EXTRACTION MACHINES & SKILL CHECKS
    // =========================================================================
    struct Machine {
        float x, y;
        float progress;          // 0.0 .. 100.0%
        bool completed;
        bool isExtracting;
        bool skillCheckActive;
        float needleAngle;       // Radians 0 .. 2*PI
        float hitZoneStart;      // Radians
        float hitZoneWidth;      // Radians
        unsigned long nextSkillCheckTime;
    };
    static const int NUM_MACHINES = 2;
    Machine machines[NUM_MACHINES];

    // =========================================================================
    // 2. HALLOWEEN TRICK-OR-TREAT BUCKETS
    // =========================================================================
    struct TrickBucket {
        float x, y;
        bool opened;
    };
    static const int NUM_BUCKETS = 2;
    TrickBucket buckets[NUM_BUCKETS];
    char bannerText[36] = "";
    unsigned long bannerUntil = 0;

    // =========================================================================
    // 3. ICHOR DROPS
    // =========================================================================
    struct IchorDrop {
        float x, y;
        bool active;
    };
    static const int MAX_ICHOR = 8;
    IchorDrop ichorList[MAX_ICHOR];
    int ichorTotal = 5;

    // =========================================================================
    // 4. TWISTEDS (MONSTERS)
    // =========================================================================
    struct TwistedEnemy {
        TwistedType type;
        TwistedAI aiState;
        float x, y;
        float prevX, prevY;
        float targetX, targetY;
        float speed;
        unsigned long stateTimer;
        unsigned long stunUntil;
        float walkPhase;
    };
    static const int MAX_TWISTEDS = 3;
    TwistedEnemy twisteds[MAX_TWISTEDS];
    int activeTwisteds = 1;

    // =========================================================================
    // 5. HALLOWEEN BLACKOUT EVENT
    // =========================================================================
    bool blackoutActive = false;
    unsigned long blackoutUntil = 0;

    // =========================================================================
    // BUTTONS
    // =========================================================================
    const int BTN_STUN_X = 26;
    const int BTN_STUN_Y = 302;
    const int BTN_STUN_R = 16;

    const int BTN_RUSH_X = 214;
    const int BTN_RUSH_Y = 302;
    const int BTN_RUSH_R = 16;

    // -------------------------------------------------------------------------
    // Setup Floor
    // -------------------------------------------------------------------------
    void setupFloor(int floor) {
        floorLevel = floor;
        gx = 120.0f;
        gy = 245.0f;
        prevGx = gx;
        prevGy = gy;
        targetX = gx;
        targetY = gy;
        elevatorUnlocked = false;
        stamina = MAX_STAMINA;
        bannerUntil = 0;

        // Blackout event on Floor 2+ (25% chance)
        blackoutActive = (floorLevel >= 2) && (random(100) < 30);
        blackoutUntil = blackoutActive ? (millis() + 10000) : 0;

        // Initialize 2 Extraction Machines
        machines[0].x = 75.0f;
        machines[0].y = 145.0f;
        machines[0].progress = 0.0f;
        machines[0].completed = false;
        machines[0].isExtracting = false;
        machines[0].skillCheckActive = false;
        machines[0].nextSkillCheckTime = millis() + random(3000, 5000);

        machines[1].x = 165.0f;
        machines[1].y = 145.0f;
        machines[1].progress = 0.0f;
        machines[1].completed = false;
        machines[1].isExtracting = false;
        machines[1].skillCheckActive = false;
        machines[1].nextSkillCheckTime = millis() + random(3000, 5000);

        // Trick or Treat Buckets
        buckets[0].x = 36.0f;
        buckets[0].y = 68.0f;
        buckets[0].opened = false;

        buckets[1].x = 204.0f;
        buckets[1].y = 230.0f;
        buckets[1].opened = false;

        // Ichor Spawn Points
        ichorTotal = min(MAX_ICHOR, 4 + floorLevel);
        const float spawnCoords[8][2] = {
            { 30, 85 },  { 210, 85 },
            { 80, 95 },  { 160, 95 },
            { 32, 160 }, { 208, 160 },
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

        // Twisteds setup based on floor
        activeTwisteds = constrain(1 + (floorLevel - 1) / 2, 1, MAX_TWISTEDS);
        for (int i = 0; i < activeTwisteds; i++) {
            twisteds[i].type = (TwistedType)(i % 3);
            twisteds[i].aiState = AI_PATROL;
            twisteds[i].x = 45.0f + i * 75.0f;
            twisteds[i].y = 75.0f + (i % 2) * 40.0f;
            twisteds[i].prevX = twisteds[i].x;
            twisteds[i].prevY = twisteds[i].y;
            twisteds[i].targetX = twisteds[i].x;
            twisteds[i].targetY = twisteds[i].y;
            twisteds[i].speed = 1.15f + (floorLevel * 0.12f) + (twisteds[i].type == TWISTED_POPPY ? 0.35f : 0.0f);
            twisteds[i].stateTimer = millis() + random(2000, 4000);
            twisteds[i].stunUntil = 0;
            twisteds[i].walkPhase = 0.0f;
        }
    }

    void resetGame() {
        score = 0;
        lives = 3;
        ichorWallet = 0;
        candiesStocked = 1;
        flashlightCharges = 2;
        sugarRushUntil = 0;
        invulnerableUntil = 0;
        setupFloor(1);
    }

    // -------------------------------------------------------------------------
    // Collision Checks
    // -------------------------------------------------------------------------
    bool checkObstacleCollision(float nx, float ny, float radius) {
        for (int i = 0; i < NUM_OBSTACLES; i++) {
            const Obstacle& ob = obstacles[i];
            if (nx + radius > ob.x && nx - radius < ob.x + ob.w &&
                ny + radius > ob.y && ny - radius < ob.y + ob.h) {
                return true;
            }
        }
        // Machine body collision
        for (int i = 0; i < NUM_MACHINES; i++) {
            if (nx + radius > machines[i].x - 16 && nx - radius < machines[i].x + 16 &&
                ny + radius > machines[i].y - 12 && ny - radius < machines[i].y + 12) {
                return true;
            }
        }
        return false;
    }

    // -------------------------------------------------------------------------
    // Alerts all Twisteds to investigate a noise location (failed skill check, etc)
    // -------------------------------------------------------------------------
    void alertTwistedsNoise(float nx, float ny) {
        for (int i = 0; i < activeTwisteds; i++) {
            if (twisteds[i].aiState != AI_STUNNED) {
                twisteds[i].aiState = AI_INVESTIGATE;
                twisteds[i].targetX = nx;
                twisteds[i].targetY = ny;
                twisteds[i].stateTimer = millis() + 4500;
            }
        }
    }

    // -------------------------------------------------------------------------
    // Drawing Helpers
    // -------------------------------------------------------------------------
    void drawGourdy(TFT_eSPI& tft, int cx, int cy, float angle, bool walking, bool sugarRush, bool blink) {
        if (blink) return;

        const uint16_t bg = 0x0842;

        // 1. Sugar Rush Golden / Magenta Aura
        if (sugarRush) {
            UI_AA::drawCircle(tft, cx, cy + 2, 16, TFT_YELLOW, bg);
            UI_AA::drawCircle(tft, cx, cy + 2, 17, TFT_MAGENTA, bg);
        }

        // 2. Long Green Vine Tail with Leaf Tip (Swishes behind movement direction)
        float tailAngle = angle + 3.14159f;
        float tailWag = walking ? sinf(walkAnimPhase) * 4.0f : 0.0f;
        int tailMidX = cx + (int)(cosf(tailAngle) * 7.0f);
        int tailMidY = cy + 6 + (int)(sinf(tailAngle) * 4.0f + tailWag);
        int tailTipX = cx + (int)(cosf(tailAngle) * 14.0f);
        int tailTipY = cy + 4 + (int)(sinf(tailAngle) * 7.0f + tailWag * 1.5f);

        tft.drawLine(cx, cy + 6, tailMidX, tailMidY, 0x1462); // Darker green base
        tft.drawLine(tailMidX, tailMidY, tailTipX, tailTipY, 0x24C5); // Bright green vine
        tft.fillTriangle(tailTipX, tailTipY - 2, tailTipX + 3, tailTipY, tailTipX, tailTipY + 2, 0x47E0); // Leaf tip

        // 3. Green Vine Legs with Vine Stripes & Running Animation
        int legOff1 = walking ? (int)(sinf(walkAnimPhase) * 3.5f) : 0;
        int legOff2 = walking ? (int)(-sinf(walkAnimPhase) * 3.5f) : 0;

        // Left Leg
        tft.fillRect(cx - 5, cy + 10 + legOff1, 3, 6, 0x24C5);
        tft.drawFastHLine(cx - 5, cy + 12 + legOff1, 3, 0x1462); // Vine stripe
        tft.fillRect(cx - 6, cy + 15 + legOff1, 4, 2, 0x1462); // Vine claw foot

        // Right Leg
        tft.fillRect(cx + 2, cy + 10 + legOff2, 3, 6, 0x24C5);
        tft.drawFastHLine(cx + 2, cy + 12 + legOff2, 3, 0x1462);
        tft.fillRect(cx + 2, cy + 15 + legOff2, 4, 2, 0x1462);

        // 4. Round Orange Pumpkin Torso with Ribbing
        UI_AA::fillCircle(tft, cx, cy + 6, 6, 0xFA40, bg);
        tft.drawFastVLine(cx - 2, cy + 2, 8, 0xD280);
        tft.drawFastVLine(cx + 2, cy + 2, 8, 0xD280);

        // 5. Green Vine Arms & Large Orange Paw Hands with Claws
        int armSwing1 = walking ? (int)(cosf(walkAnimPhase) * 2.5f) : 0;
        int armSwing2 = walking ? (int)(-cosf(walkAnimPhase) * 2.5f) : 0;

        // Left Arm & Paw
        int paw1X = cx - 9 + armSwing1;
        int paw1Y = cy + 6 - armSwing1;
        tft.drawLine(cx - 4, cy + 5, paw1X, paw1Y, 0x24C5);
        UI_AA::fillCircle(tft, paw1X, paw1Y, 3, 0xFD20, bg);
        tft.drawPixel(paw1X - 2, paw1Y - 1, 0x1462); // Claws
        tft.drawPixel(paw1X - 1, paw1Y - 2, 0x1462);
        tft.drawPixel(paw1X, paw1Y, 0x9300); // Paw pad center

        // Right Arm & Paw
        int paw2X = cx + 9 + armSwing2;
        int paw2Y = cy + 6 + armSwing2;
        tft.drawLine(cx + 4, cy + 5, paw2X, paw2Y, 0x24C5);
        UI_AA::fillCircle(tft, paw2X, paw2Y, 3, 0xFD20, bg);
        tft.drawPixel(paw2X + 2, paw2Y - 1, 0x1462);
        tft.drawPixel(paw2X + 1, paw2Y - 2, 0x1462);
        tft.drawPixel(paw2X, paw2Y, 0x9300);

        // 6. Giant Jack-o'-Lantern Pumpkin Head (Composite Lobes)
        UI_AA::fillCircle(tft, cx - 4, cy - 2, 7, 0xFA40, bg);
        UI_AA::fillCircle(tft, cx + 4, cy - 2, 7, 0xFA40, bg);
        UI_AA::fillCircle(tft, cx, cy - 2, 8, 0xFD20, bg);

        // Vertical rib grooves
        tft.drawFastVLine(cx - 3, cy - 8, 12, 0xD280);
        tft.drawFastVLine(cx + 3, cy - 8, 12, 0xD280);

        // Distinct carved horizontal lid seam across upper forehead
        tft.drawFastHLine(cx - 6, cy - 4, 13, 0x9260);
        tft.drawPixel(cx - 2, cy - 5, 0x9260);
        tft.drawPixel(cx + 2, cy - 5, 0x9260);

        // Woody brown stem on top of lid
        tft.fillRect(cx - 1, cy - 13, 3, 4, 0x69A1);
        tft.drawPixel(cx - 1, cy - 14, 0x82C2);

        // Curled green leaf attached to stem
        tft.fillRect(cx + 2, cy - 13, 4, 2, 0x24C5);
        tft.drawPixel(cx + 4, cy - 14, 0x47E0);
        tft.drawPixel(cx + 6, cy - 12, 0x24C5);

        // 7. Carved Hollow Black Eye Sockets with Tracking White Pupils
        tft.fillRect(cx - 5, cy - 6, 3, 3, TFT_BLACK);
        tft.fillRect(cx + 2, cy - 6, 3, 3, TFT_BLACK);
        int lookDx = (int)constrain(cosf(angle) * 1.5f, -1.0f, 1.0f);
        int lookDy = (int)constrain(sinf(angle) * 1.5f, -1.0f, 1.0f);
        tft.drawPixel(cx - 4 + lookDx, cy - 5 + lookDy, TFT_WHITE);
        tft.drawPixel(cx + 3 + lookDx, cy - 5 + lookDy, TFT_WHITE);

        // 8. Mischievous Fanged Grin
        tft.drawFastHLine(cx - 4, cy, 9, TFT_BLACK);
        tft.drawPixel(cx - 3, cy + 1, TFT_BLACK);
        tft.drawPixel(cx + 3, cy + 1, TFT_BLACK);
        // Pointed white fangs
        tft.drawPixel(cx - 2, cy, TFT_WHITE);
        tft.drawPixel(cx + 2, cy, TFT_WHITE);
        tft.drawPixel(cx, cy + 1, TFT_WHITE);
    }

    void drawTwisted(TFT_eSPI& tft, const TwistedEnemy& tw) {
        int cx = (int)tw.x;
        int cy = (int)tw.y;
        const uint16_t bg = 0x0842;

        // Status indicator above head
        if (tw.aiState == AI_CHASE) {
            tft.setTextColor(TFT_RED, bg);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("!", cx, cy - 17, 2);
        } else if (tw.aiState == AI_INVESTIGATE) {
            tft.setTextColor(TFT_YELLOW, bg);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("?", cx, cy - 17, 2);
        } else if (tw.aiState == AI_STUNNED) {
            tft.setTextColor(TFT_YELLOW, bg);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("***", cx, cy - 17, 1);
        }

        // Inky Dripping Tendrils
        tft.drawFastVLine(cx - 7, cy + 7, 6, TFT_BLACK);
        tft.drawFastVLine(cx - 2, cy + 8, 7, TFT_BLACK);
        tft.drawFastVLine(cx + 3, cy + 7, 6, TFT_BLACK);
        tft.drawFastVLine(cx + 7, cy + 6, 5, TFT_BLACK);

        // Corrupted Inky Head
        UI_AA::fillCircle(tft, cx, cy, 9, 0x1082, bg);
        UI_AA::drawCircle(tft, cx, cy, 9, TFT_BLACK, bg);

        if (tw.type == TWISTED_DANDY) {
            // Corrupted Pumpkin Flower Petals
            for (int p = 0; p < 6; p++) {
                float rad = p * 1.047f;
                int px = cx + (int)(cosf(rad) * 11.0f);
                int py = cy + (int)(sinf(rad) * 11.0f);
                tft.drawPixel(px, py, 0xFA40);
            }
        } else if (tw.type == TWISTED_POPPY) {
            // Ink Bubbles
            tft.drawCircle(cx - 7, cy - 8, 3, 0xD81F);
        } else {
            // Boxten Box corners
            tft.drawRect(cx - 7, cy - 7, 14, 14, 0x7BEF);
        }

        // Piercing Eyes
        uint16_t eyeCol = (tw.aiState == AI_CHASE) ? TFT_RED : (tw.aiState == AI_INVESTIGATE ? TFT_YELLOW : TFT_WHITE);
        tft.fillRect(cx - 5, cy - 2, 3, 3, eyeCol);
        tft.fillRect(cx + 2, cy - 2, 3, 3, eyeCol);
        tft.drawPixel(cx - 4, cy - 1, TFT_BLACK);
        tft.drawPixel(cx + 3, cy - 1, TFT_BLACK);
    }

    void drawMachine(TFT_eSPI& tft, int idx) {
        const Machine& m = machines[idx];
        int bx = (int)m.x;
        int by = (int)m.y;

        // Computer Terminal Frame
        UI_AA::fillRoundRect(tft, bx - 16, by - 14, 32, 28, 4, 0x18C3, 0x0842);
        UI_AA::drawRoundRect(tft, bx - 16, by - 14, 32, 28, 4, m.completed ? TFT_GREEN : 0x5AEB, 0x0842);

        // Screen
        uint16_t screenBg = m.completed ? 0x0320 : 0x0145;
        tft.fillRect(bx - 12, by - 10, 24, 14, screenBg);

        tft.setTextDatum(MC_DATUM);
        char pbuf[16];
        snprintf(pbuf, sizeof(pbuf), "%d%%", (int)m.progress);
        tft.setTextColor(m.completed ? TFT_GREEN : TFT_CYAN, screenBg);
        tft.drawString(pbuf, bx, by - 3, 1);

        // Status Indicator LED
        uint16_t ledCol = m.completed ? TFT_GREEN : (m.isExtracting ? TFT_YELLOW : TFT_RED);
        UI_AA::fillCircle(tft, bx, by + 8, 3, ledCol, 0x18C3);

        // Skill Check Overlay
        if (m.skillCheckActive) {
            int scRadius = 18;
            UI_AA::drawCircle(tft, bx, by, scRadius, TFT_WHITE, screenBg);
            // Draw hit zone arc
            for (float a = m.hitZoneStart; a < m.hitZoneStart + m.hitZoneWidth; a += 0.12f) {
                int ax = bx + (int)(cosf(a) * scRadius);
                int ay = by + (int)(sinf(a) * scRadius);
                tft.drawPixel(ax, ay, TFT_GREEN);
            }
            // Draw Needle
            int nx = bx + (int)(cosf(m.needleAngle) * scRadius);
            int ny = by + (int)(sinf(m.needleAngle) * scRadius);
            tft.drawLine(bx, by, nx, ny, TFT_YELLOW);

            tft.setTextColor(TFT_YELLOW, 0x0842);
            tft.drawString("TAP!", bx, by - 22, 1);
        }
    }

    void drawTrickBucket(TFT_eSPI& tft, int idx) {
        const TrickBucket& b = buckets[idx];
        int bx = (int)b.x;
        int by = (int)b.y;
        const uint16_t bg = 0x0842;

        if (b.opened) {
            // Opened bucket (spent)
            UI_AA::drawCircle(tft, bx, by, 7, 0x632C, bg);
            tft.setTextDatum(MC_DATUM);
            tft.setTextColor(0x632C, bg);
            tft.drawString("EMPTY", bx, by + 11, 1);
        } else {
            // Bright Halloween Jack-o'-lantern candy bucket
            UI_AA::fillCircle(tft, bx, by, 8, TFT_ORANGE, bg);
            UI_AA::drawCircle(tft, bx, by, 8, TFT_YELLOW, bg);
            // Black handle
            tft.drawCircle(bx, by - 4, 7, TFT_BLACK);
            // Carved face
            tft.drawPixel(bx - 3, by - 1, TFT_BLACK);
            tft.drawPixel(bx + 3, by - 1, TFT_BLACK);
            tft.drawFastHLine(bx - 3, by + 3, 7, TFT_BLACK);
            tft.setTextDatum(MC_DATUM);
            tft.setTextColor(TFT_YELLOW, bg);
            tft.drawString("TREAT", bx, by + 12, 1);
        }
    }

    void drawIchor(TFT_eSPI& tft, int ix, int iy) {
        const uint16_t bg = 0x0842;
        UI_AA::fillCircle(tft, ix, iy, 5, 0xD81F, bg);
        UI_AA::fillCircle(tft, ix, iy, 2, TFT_WHITE, 0xD81F);
    }

    void drawWorldStatic(TFT_eSPI& tft) {
        tft.fillScreen(0x0842);

        // Arena outer frame
        tft.drawRect(ARENA_LEFT, ARENA_TOP, ARENA_RIGHT - ARENA_LEFT, ARENA_BOTTOM - ARENA_TOP, 0x2124);

        // Top Elevator Door
        uint16_t elevBorder = elevatorUnlocked ? TFT_GREEN : 0x6B4D;
        uint16_t elevFill = elevatorUnlocked ? 0x0240 : 0x18C3;
        UI_AA::fillRoundRect(tft, ELEV_X - ELEV_W / 2, ELEV_Y, ELEV_W, ELEV_H, 3, elevFill, 0x0842);
        UI_AA::drawRoundRect(tft, ELEV_X - ELEV_W / 2, ELEV_Y, ELEV_W, ELEV_H, 3, elevBorder, 0x0842);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(elevBorder, elevFill);
        tft.drawString(elevatorUnlocked ? "OPEN" : "ELEVATOR", ELEV_X, ELEV_Y + 7, 1);

        // Obstacles (Gardenview Stone Pillars)
        for (int i = 0; i < NUM_OBSTACLES; i++) {
            const Obstacle& ob = obstacles[i];
            UI_AA::fillRoundRect(tft, ob.x, ob.y, ob.w, ob.h, 4, 0x18C3, 0x0842);
            UI_AA::drawRoundRect(tft, ob.x, ob.y, ob.w, ob.h, 4, 0x4228, 0x0842);
            tft.fillRect(ob.x + 3, ob.y + 3, ob.w - 6, ob.h - 6, 0x1082);
        }

        // Draw Machines
        for (int i = 0; i < NUM_MACHINES; i++) {
            drawMachine(tft, i);
        }

        // Draw Trick Buckets
        for (int i = 0; i < NUM_BUCKETS; i++) {
            drawTrickBucket(tft, i);
        }
    }

    void drawHUD(TFT_eSPI& tft) {
        // Header bar (y: 26..50)
        tft.fillRect(0, 26, 240, 24, 0x1082);
        tft.drawFastHLine(0, 50, 240, 0x2965);

        // < ARC Button
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
        snprintf(buf, sizeof(buf), "ICHOR:%d", ichorWallet);
        tft.setTextColor(0xD81F, 0x1082);
        tft.drawString(buf, 86, 30, 2);

        // Gourdy Lives (Pumpkin hearts)
        for (int i = 0; i < 3; i++) {
            int hx = 186 + i * 16;
            if (i < lives) {
                UI_AA::fillCircle(tft, hx, 38, 5, 0xFD20, 0x1082);
                tft.drawPixel(hx, 34, 0x24C5);
            } else {
                UI_AA::drawCircle(tft, hx, 38, 5, 0x52AA, 0x1082);
            }
        }

        // Bottom Toolbar (y: 284..320)
        tft.fillRect(0, 284, 240, 36, 0x1082);
        tft.drawFastHLine(0, 284, 240, 0x2965);

        // 1. Flashlight Stun Button (Left)
        bool stunReady = (flashlightCharges > 0) && (millis() > flashlightCooldown);
        uint16_t stunCol = stunReady ? TFT_YELLOW : 0x3186;
        UI_AA::fillCircle(tft, BTN_STUN_X, BTN_STUN_Y, BTN_STUN_R, stunCol, 0x1082);
        UI_AA::drawCircle(tft, BTN_STUN_X, BTN_STUN_Y, BTN_STUN_R, TFT_WHITE, 0x1082);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_BLACK, stunCol);
        char sbuf[8];
        snprintf(sbuf, sizeof(sbuf), "STUN(%d)", flashlightCharges);
        tft.drawString(sbuf, BTN_STUN_X, BTN_STUN_Y, 1);

        // 2. Stamina Meter (Center)
        tft.setTextColor(0x94B2, 0x1082);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("STAMINA", 120, 292, 1);
        tft.drawRect(58, 301, 124, 8, TFT_WHITE);
        int stamW = (int)((stamina * 122.0f) / MAX_STAMINA);
        tft.fillRect(59, 302, stamW, 6, stamina > 30.0f ? TFT_GREEN : TFT_RED);
        tft.fillRect(59 + stamW, 302, 122 - stamW, 6, TFT_BLACK);

        // 3. Sugar Rush Candy Button (Right)
        bool hasBoost = (candiesStocked > 0) || (millis() < sugarRushUntil);
        uint16_t btnCol = (millis() < sugarRushUntil) ? TFT_MAGENTA : (hasBoost ? TFT_ORANGE : 0x3186);
        UI_AA::fillCircle(tft, BTN_RUSH_X, BTN_RUSH_Y, BTN_RUSH_R, btnCol, 0x1082);
        UI_AA::drawCircle(tft, BTN_RUSH_X, BTN_RUSH_Y, BTN_RUSH_R, TFT_WHITE, 0x1082);
        tft.setTextColor(TFT_WHITE, btnCol);
        char cbuf[8];
        snprintf(cbuf, sizeof(cbuf), "RUSH(%d)", candiesStocked);
        tft.drawString((millis() < sugarRushUntil) ? "BOOST!" : cbuf, BTN_RUSH_X, BTN_RUSH_Y, 1);
    }

    void drawBanner(TFT_eSPI& tft) {
        if (millis() < bannerUntil) {
            UI_AA::fillRoundRect(tft, 20, 68, 200, 26, 4, 0x2A15, 0x0842);
            UI_AA::drawRoundRect(tft, 20, 68, 200, 26, 4, TFT_YELLOW, 0x0842);
            tft.setTextDatum(MC_DATUM);
            tft.setTextColor(TFT_WHITE, 0x2A15);
            tft.drawString(bannerText, 120, 81, 2);
        }
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
        tft.drawString("DANDY'S WORLD", 140, 38, 2);

        // Big Gourdy Illustration
        drawGourdy(tft, 120, 95, -1.57f, false, true, false);

        // Titles
        tft.setTextColor(TFT_ORANGE, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("GARDENVIEW", 120, 126, 4);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawString("HALLOWEEN", 120, 148, 4);

        // Gourdy Stats (Authentic Dandy's World Star Ratings)
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(0xD81F, TFT_BLACK);
        tft.drawString("GOURDY'S TOON STATS:", 34, 172, 2);
        tft.setTextColor(0xAD75, TFT_BLACK);
        tft.drawString("Speed:  ***** (Sugar Rush)", 34, 192, 1);
        tft.drawString("Stamina: ****  Stealth: *", 34, 204, 1);
        tft.drawString("Extract: ***   Skill Check: **", 34, 216, 1);

        // High Score
        char hbuf[32];
        snprintf(hbuf, sizeof(hbuf), "High Score: %u", (unsigned)highScore);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawString(hbuf, 60, 234, 2);

        // Start Button
        UI_AA::fillRoundRect(tft, 25, 258, 190, 42, 6, TFT_GREEN, TFT_BLACK);
        UI_AA::drawRoundRect(tft, 25, 258, 190, 42, 6, TFT_WHITE, TFT_BLACK);
        tft.setTextColor(TFT_BLACK, TFT_GREEN);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("ENTER GARDENVIEW", 120, 279, 4);
    }

    void drawDandyShop(TFT_eSPI& tft) {
        tft.fillScreen(0x0821);

        // Header
        tft.fillRect(0, 26, 240, 24, 0x1082);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_YELLOW, 0x1082);
        tft.drawString("DANDY'S ELEVATOR SHOP", 120, 38, 2);

        // Dandy Illustration (With Halloween Top Hat & Bowtie)
        int dx = 120, dy = 80;
        // Flower Petals
        for (int i = 0; i < 6; i++) {
            float a = i * 1.047f;
            UI_AA::fillCircle(tft, dx + (int)(cosf(a) * 16), dy + (int)(sinf(a) * 16), 8, TFT_YELLOW, 0x0821);
        }
        UI_AA::fillCircle(tft, dx, dy, 12, TFT_WHITE, 0x0821);
        tft.drawPixel(dx - 4, dy - 2, TFT_BLACK);
        tft.drawPixel(dx + 4, dy - 2, TFT_BLACK);
        tft.drawFastHLine(dx - 5, dy + 4, 11, TFT_BLACK);
        // Halloween Witch/Top Hat
        tft.fillRect(dx - 14, dy - 16, 28, 4, 0x3810);
        tft.fillRect(dx - 8, dy - 26, 16, 10, 0x3810);
        tft.drawFastHLine(dx - 8, dy - 18, 16, TFT_ORANGE);

        // Dandy Speech Bubble
        tft.setTextColor(TFT_WHITE, 0x0821);
        tft.drawString("\"Happy Halloween, Gourdy!\"", 120, 115, 2);
        char ibuf[32];
        snprintf(ibuf, sizeof(ibuf), "Your Ichor: %d", ichorWallet);
        tft.setTextColor(0xD81F, 0x0821);
        tft.drawString(ibuf, 120, 132, 2);

        // Shop Item 1: Chocolate Bar (+1 Heart) - Cost: 8 Ichor
        UI_AA::fillRoundRect(tft, 14, 150, 212, 34, 4, 0x18C3, 0x0821);
        UI_AA::drawRoundRect(tft, 14, 150, 212, 34, 4, (ichorWallet >= 8) ? TFT_GREEN : 0x4228, 0x0821);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_WHITE, 0x18C3);
        tft.drawString("1. CHOCOLATE BAR (+1 Heart)", 22, 154, 2);
        tft.setTextColor(0xD81F, 0x18C3);
        tft.drawString("Cost: 8 Ichor [BUY]", 22, 168, 1);

        // Shop Item 2: Candy Corn (+1 Sugar Rush) - Cost: 5 Ichor
        UI_AA::fillRoundRect(tft, 14, 190, 212, 34, 4, 0x18C3, 0x0821);
        UI_AA::drawRoundRect(tft, 14, 190, 212, 34, 4, (ichorWallet >= 5) ? TFT_ORANGE : 0x4228, 0x0821);
        tft.setTextColor(TFT_WHITE, 0x18C3);
        tft.drawString("2. CANDY CORN (+1 Speed Boost)", 22, 194, 2);
        tft.setTextColor(0xD81F, 0x18C3);
        tft.drawString("Cost: 5 Ichor [BUY]", 22, 208, 1);

        // Shop Item 3: Flashlight Battery (+2 Stuns) - Cost: 6 Ichor
        UI_AA::fillRoundRect(tft, 14, 230, 212, 34, 4, 0x18C3, 0x0821);
        UI_AA::drawRoundRect(tft, 14, 230, 212, 34, 4, (ichorWallet >= 6) ? TFT_YELLOW : 0x4228, 0x0821);
        tft.setTextColor(TFT_WHITE, 0x18C3);
        tft.drawString("3. FLASHLIGHT BATTERY (+2 Stun)", 22, 234, 2);
        tft.setTextColor(0xD81F, 0x18C3);
        tft.drawString("Cost: 6 Ichor [BUY]", 22, 248, 1);

        // Continue Button
        UI_AA::fillRoundRect(tft, 25, 274, 190, 36, 6, TFT_CYAN, 0x0821);
        UI_AA::drawRoundRect(tft, 25, 274, 190, 36, 6, TFT_WHITE, 0x0821);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_BLACK, TFT_CYAN);
        char cbuf[32];
        snprintf(cbuf, sizeof(cbuf), "ENTER FLOOR %d >", floorLevel + 1);
        tft.drawString(cbuf, 120, 292, 4);
    }

    void onUpdate(TFT_eSPI& tft, const TouchPoint& touch, unsigned long dt) {
        // ---------------------------------------------------------------------
        // Title Screen Interaction
        // ---------------------------------------------------------------------
        if (state == STATE_TITLE) {
            if (touch.justPressed && touch.y > 50) {
                resetGame();
                state = STATE_PLAYING;
                drawWorldStatic(tft);
                drawHUD(tft);
            }
            return;
        }

        // ---------------------------------------------------------------------
        // Dandy's Halloween Shop Interaction
        // ---------------------------------------------------------------------
        if (state == STATE_DANDY_SHOP) {
            if (touch.justPressed) {
                // Item 1: Chocolate Bar
                if (touch.y >= 150 && touch.y <= 184 && ichorWallet >= 8) {
                    ichorWallet -= 8;
                    lives = min(3, lives + 1);
                    drawDandyShop(tft);
                }
                // Item 2: Candy Corn
                else if (touch.y >= 190 && touch.y <= 224 && ichorWallet >= 5) {
                    ichorWallet -= 5;
                    candiesStocked++;
                    drawDandyShop(tft);
                }
                // Item 3: Flashlight Battery
                else if (touch.y >= 230 && touch.y <= 264 && ichorWallet >= 6) {
                    ichorWallet -= 6;
                    flashlightCharges += 2;
                    drawDandyShop(tft);
                }
                // Continue to Next Floor
                else if (touch.y >= 274 && touch.y <= 312) {
                    setupFloor(floorLevel + 1);
                    state = STATE_PLAYING;
                    drawWorldStatic(tft);
                    drawHUD(tft);
                }
            }
            return;
        }

        // ---------------------------------------------------------------------
        // Game Over Interaction
        // ---------------------------------------------------------------------
        if (state == STATE_GAME_OVER) {
            if (touch.justPressed && touch.y > 50) {
                resetGame();
                state = STATE_PLAYING;
                drawWorldStatic(tft);
                drawHUD(tft);
            }
            return;
        }

        // ---------------------------------------------------------------------
        // Floor Clear Cutscene
        // ---------------------------------------------------------------------
        if (state == STATE_FLOOR_CLEAR) {
            static unsigned long clearTimer = 0;
            if (clearTimer == 0) clearTimer = millis();
            if (millis() - clearTimer > 1200) {
                clearTimer = 0;
                state = STATE_DANDY_SHOP;
                drawDandyShop(tft);
            }
            return;
        }

        // =====================================================================
        // PLAYING STATE
        // =====================================================================

        // 1. Touch Input: Flashlight, Sugar Rush, or Arena Movement
        if (touch.pressed) {
            // Flashlight Stun Button (x: 10..42, y: 286..318)
            int sdx = (int)touch.x - BTN_STUN_X;
            int sdy = (int)touch.y - BTN_STUN_Y;
            if (sdx * sdx + sdy * sdy <= (BTN_STUN_R + 6) * (BTN_STUN_R + 6)) {
                if (touch.justPressed && flashlightCharges > 0 && millis() > flashlightCooldown) {
                    flashlightCharges--;
                    flashlightCooldown = millis() + 4000;
                    stunFlashVisualUntil = millis() + 300;

                    // Stun Twisteds in front of Gourdy
                    for (int i = 0; i < activeTwisteds; i++) {
                        float tdx = twisteds[i].x - gx;
                        float tdy = twisteds[i].y - gy;
                        float dist = sqrtf(tdx * tdx + tdy * tdy);
                        if (dist < 80.0f) {
                            twisteds[i].aiState = AI_STUNNED;
                            twisteds[i].stunUntil = millis() + 2500;
                        }
                    }
                    snprintf(bannerText, sizeof(bannerText), "FLASH! TWISTED STUNNED!");
                    bannerUntil = millis() + 1500;
                    drawHUD(tft);
                }
            }
            // Sugar Rush Candy button (x: 198..230, y: 286..318)
            else {
                int rdx = (int)touch.x - BTN_RUSH_X;
                int rdy = (int)touch.y - BTN_RUSH_Y;
                if (rdx * rdx + rdy * rdy <= (BTN_RUSH_R + 6) * (BTN_RUSH_R + 6)) {
                    if (touch.justPressed && candiesStocked > 0 && millis() > sugarRushUntil) {
                        candiesStocked--;
                        sugarRushUntil = millis() + 4500;
                        snprintf(bannerText, sizeof(bannerText), "SUGAR RUSH ACTIVATED!");
                        bannerUntil = millis() + 1500;
                        drawHUD(tft);
                    }
                }
                // Arena Steering
                else if (touch.y >= ARENA_TOP && touch.y <= ARENA_BOTTOM) {
                    targetX = constrain((float)touch.x, (float)ARENA_LEFT + 12, (float)ARENA_RIGHT - 12);
                    targetY = constrain((float)touch.y, (float)ARENA_TOP + 12, (float)ARENA_BOTTOM - 12);
                }
            }
        }

        // 2. Gourdy Physics & Movement
        bool hasRush = (millis() < sugarRushUntil);
        float currentSpeed = hasRush ? 3.5f : 2.3f;

        float dx = targetX - gx;
        float dy = targetY - gy;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist > 3.0f) {
            isMoving = true;
            facingAngle = atan2f(dy, dx);
            walkAnimPhase += 0.35f;

            if (stamina > 0.0f) {
                stamina = max(0.0f, stamina - (hasRush ? 0.08f : 0.04f));
            } else {
                currentSpeed *= 0.65f;
            }

            float stepX = (dx / dist) * min(dist, currentSpeed);
            float stepY = (dy / dist) * min(dist, currentSpeed);
            float nextX = gx + stepX;
            float nextY = gy + stepY;

            if (!checkObstacleCollision(nextX, gy, 8.0f)) {
                gx = constrain(nextX, (float)ARENA_LEFT + 8, (float)ARENA_RIGHT - 8);
            }
            if (!checkObstacleCollision(gx, nextY, 8.0f)) {
                gy = constrain(nextY, (float)ARENA_TOP + 8, (float)ARENA_BOTTOM - 8);
            }
        } else {
            isMoving = false;
            stamina = min(MAX_STAMINA, stamina + 0.20f);
        }

        // 3. Extraction Machines & Skill Checks
        bool allCompleted = true;
        for (int i = 0; i < NUM_MACHINES; i++) {
            Machine& m = machines[i];
            if (!m.completed) allCompleted = false;

            float mDist = hypotf(gx - m.x, gy - m.y);
            bool inRange = (mDist < 26.0f);
            m.isExtracting = inRange && !m.completed;

            if (m.isExtracting) {
                // Extracting progress
                m.progress = min(100.0f, m.progress + (dt * 0.015f));
                score += 1;

                if (m.progress >= 100.0f) {
                    m.completed = true;
                    m.skillCheckActive = false;
                    score += 200;
                    snprintf(bannerText, sizeof(bannerText), "GENERATOR %d EXTRACTED!", i + 1);
                    bannerUntil = millis() + 1800;
                }

                // Trigger Skill Check QTE
                if (!m.skillCheckActive && millis() > m.nextSkillCheckTime && !m.completed) {
                    m.skillCheckActive = true;
                    m.needleAngle = 0.0f;
                    m.hitZoneStart = random(10, 45) * 0.1f;
                    m.hitZoneWidth = 0.65f;
                }

                // Skill Check Needle Movement & Input
                if (m.skillCheckActive) {
                    m.needleAngle += dt * 0.007f;

                    // Tap during skill check
                    if (touch.justPressed) {
                        float diff = fabsf(m.needleAngle - (m.hitZoneStart + m.hitZoneWidth / 2.0f));
                        if (diff <= m.hitZoneWidth / 2.0f) {
                            // GREAT / GOOD HIT!
                            m.progress = min(100.0f, m.progress + 14.0f);
                            score += 50;
                            snprintf(bannerText, sizeof(bannerText), "SKILL CHECK: GREAT HIT!");
                            bannerUntil = millis() + 1000;
                        } else {
                            // FAILED HIT!
                            m.progress = max(0.0f, m.progress - 5.0f);
                            snprintf(bannerText, sizeof(bannerText), "SKILL CHECK FAILED!");
                            bannerUntil = millis() + 1200;
                            alertTwistedsNoise(m.x, m.y); // Alert Twisteds!
                        }
                        m.skillCheckActive = false;
                        m.nextSkillCheckTime = millis() + random(3500, 5500);
                    } else if (m.needleAngle >= 6.28f) {
                        // Timed out missed needle
                        m.progress = max(0.0f, m.progress - 5.0f);
                        snprintf(bannerText, sizeof(bannerText), "MISSED SKILL CHECK!");
                        bannerUntil = millis() + 1200;
                        alertTwistedsNoise(m.x, m.y);
                        m.skillCheckActive = false;
                        m.nextSkillCheckTime = millis() + random(3500, 5500);
                    }
                }
            } else {
                m.skillCheckActive = false;
            }
        }

        // Elevator Door Unlocking
        if (allCompleted && !elevatorUnlocked) {
            elevatorUnlocked = true;
            snprintf(bannerText, sizeof(bannerText), "ELEVATOR UNLOCKED! ESCAPE!");
            bannerUntil = millis() + 2500;
            // Draw updated elevator
            drawWorldStatic(tft);
            drawHUD(tft);
        }

        // 4. Elevator Entry Check
        if (elevatorUnlocked && gy <= ELEV_Y + ELEV_H + 8 && fabsf(gx - ELEV_X) < ELEV_W / 2) {
            score += 500;
            state = STATE_FLOOR_CLEAR;
            UI_AA::fillRoundRect(tft, 30, 130, 180, 54, 6, 0x0320, 0x0842);
            UI_AA::drawRoundRect(tft, 30, 130, 180, 54, 6, TFT_GREEN, 0x0842);
            tft.setTextDatum(MC_DATUM);
            tft.setTextColor(TFT_GREEN, 0x0320);
            tft.drawString("FLOOR COMPLETE!", 120, 148, 2);
            tft.setTextColor(TFT_WHITE, 0x0320);
            tft.drawString("Entering Dandy's Elevator...", 120, 168, 1);
            return;
        }

        // 5. Trick-or-Treat Bucket Collisions
        for (int i = 0; i < NUM_BUCKETS; i++) {
            if (!buckets[i].opened && hypotf(gx - buckets[i].x, gy - buckets[i].y) < 18.0f) {
                buckets[i].opened = true;
                int roll = random(100);
                if (roll < 40) {
                    candiesStocked++;
                    snprintf(bannerText, sizeof(bannerText), "TREAT: CANDY CORN!");
                } else if (roll < 75) {
                    ichorWallet += 5;
                    snprintf(bannerText, sizeof(bannerText), "TREAT: +5 ICHOR!");
                } else if (roll < 90) {
                    lives = min(3, lives + 1);
                    snprintf(bannerText, sizeof(bannerText), "TREAT: CHOCOLATE (+1 HEART)!");
                } else {
                    alertTwistedsNoise(gx, gy);
                    snprintf(bannerText, sizeof(bannerText), "TRICK: TWISTEDS SPOOKED!");
                }
                bannerUntil = millis() + 1800;
                drawTrickBucket(tft, i);
                drawHUD(tft);
            }
        }

        // 6. Ichor Pickup Collisions
        for (int i = 0; i < ichorTotal; i++) {
            if (ichorList[i].active && hypotf(gx - ichorList[i].x, gy - ichorList[i].y) < 14.0f) {
                ichorList[i].active = false;
                ichorWallet += 1;
                score += 25;
                drawHUD(tft);
            }
        }

        // 7. Twisteds AI Update Loop
        for (int i = 0; i < activeTwisteds; i++) {
            TwistedEnemy& tw = twisteds[i];

            if (tw.aiState == AI_STUNNED) {
                if (millis() > tw.stunUntil) {
                    tw.aiState = AI_PATROL;
                    tw.stateTimer = millis() + random(3000, 5000);
                }
            } else {
                float tdx = gx - tw.x;
                float tdy = gy - tw.y;
                float tDist = sqrtf(tdx * tdx + tdy * tdy);

                // Detection Radius: Gourdy has low stealth (larger alert radius when moving)
                float detectRange = isMoving ? 95.0f : 65.0f;
                if (tw.type == TWISTED_DANDY) detectRange += 20.0f;

                if (tDist < detectRange) {
                    tw.aiState = AI_CHASE;
                    tw.targetX = gx;
                    tw.targetY = gy;
                } else if (tw.aiState == AI_CHASE && tDist > detectRange + 45.0f) {
                    tw.aiState = AI_INVESTIGATE;
                    tw.stateTimer = millis() + 3000;
                }

                if (tw.aiState == AI_INVESTIGATE && millis() > tw.stateTimer) {
                    tw.aiState = AI_PATROL;
                    tw.stateTimer = millis() + random(3000, 5000);
                }

                // Move Twisted towards target
                float mdx = tw.targetX - tw.x;
                float mdy = tw.targetY - tw.y;
                float mDist = sqrtf(mdx * mdx + mdy * mdy);

                if (tw.aiState == AI_PATROL && (mDist < 8.0f || millis() > tw.stateTimer)) {
                    tw.targetX = random(ARENA_LEFT + 20, ARENA_RIGHT - 20);
                    tw.targetY = random(ARENA_TOP + 30, ARENA_BOTTOM - 30);
                    tw.stateTimer = millis() + random(3000, 6000);
                }

                if (mDist > 2.0f) {
                    float step = (tw.aiState == AI_CHASE) ? (tw.speed * 1.25f) : (tw.speed * 0.75f);
                    tw.x += (mdx / mDist) * step;
                    tw.y += (mdy / mDist) * step;
                }

                // Twisted Touch Collision with Gourdy
                if (tDist < 16.0f && millis() > invulnerableUntil) {
                    lives--;
                    invulnerableUntil = millis() + 1800;
                    snprintf(bannerText, sizeof(bannerText), "TWISTED STRUCK GOURDY!");
                    bannerUntil = millis() + 1500;
                    drawHUD(tft);

                    if (lives <= 0) {
                        state = STATE_GAME_OVER;
                        if (score > highScore) highScore = score;

                        UI_AA::fillRoundRect(tft, 25, 130, 190, 78, 6, 0x8000, 0x0842);
                        UI_AA::drawRoundRect(tft, 25, 130, 190, 78, 6, TFT_RED, 0x0842);
                        tft.setTextDatum(MC_DATUM);
                        tft.setTextColor(TFT_RED, 0x8000);
                        tft.drawString("TWISTED CAUGHT YOU!", 120, 150, 2);
                        tft.setTextColor(TFT_WHITE, 0x8000);
                        char goBuf[32];
                        snprintf(goBuf, sizeof(goBuf), "Floor Reached: %d", floorLevel);
                        tft.drawString(goBuf, 120, 172, 1);
                        tft.drawString("Tap screen to try again", 120, 192, 1);
                        return;
                    }
                }
            }
        }

        // =====================================================================
        // DIFFERENTIAL 60FPS RENDERING
        // =====================================================================
        // Erase previous Gourdy
        tft.fillRect((int)prevGx - 16, (int)prevGy - 16, 32, 36, 0x0842);

        // Erase previous Twisteds
        for (int i = 0; i < activeTwisteds; i++) {
            tft.fillRect((int)twisteds[i].prevX - 14, (int)twisteds[i].prevY - 20, 29, 36, 0x0842);
        }

        // Redraw Machines
        for (int i = 0; i < NUM_MACHINES; i++) {
            drawMachine(tft, i);
        }

        // Draw Ichor drops
        for (int i = 0; i < ichorTotal; i++) {
            if (ichorList[i].active) {
                drawIchor(tft, (int)ichorList[i].x, (int)ichorList[i].y);
            }
        }

        // Draw Twisteds
        for (int i = 0; i < activeTwisteds; i++) {
            drawTwisted(tft, twisteds[i]);
            twisteds[i].prevX = twisteds[i].x;
            twisteds[i].prevY = twisteds[i].y;
        }

        // Draw Gourdy
        bool blink = (millis() < invulnerableUntil) && ((millis() / 150) % 2 == 0);
        drawGourdy(tft, (int)gx, (int)gy, facingAngle, isMoving, hasRush, blink);
        prevGx = gx;
        prevGy = gy;

        // Draw Banner if active
        drawBanner(tft);
    }

    void onExit(TFT_eSPI& tft) {}
};
