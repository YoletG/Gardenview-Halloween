#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <xpt2046.h>
#include "pins.h"
#include "data.h"
#include "AppManager.h"
#include "AppDrawingPad.h"
#include "AppArcade.h"
#include "AppSystemMonitor.h"

// Hardware instances
TFT_eSPI tft = TFT_eSPI();
XPT2046 touch = XPT2046(SPI, TOUCHSCREEN_CS_PIN, TOUCHSCREEN_IRQ_PIN);

// Multi-app instances
AppDrawingPad drawingPadApp;
AppArcade arcadeApp;
AppSystemMonitor sysMonApp;

AppManager appManager(tft, touch);

unsigned long lastLoopTime = 0;

void setBacklight(uint8_t value) {
    static uint8_t steps = 16;
    static uint8_t _brightness = 0;

    if (_brightness == value) return;
    if (value > 16) value = 16;
    
    if (value == 0) {
        digitalWrite(BK_LIGHT_PIN, 0);
        delay(3);
        _brightness = 0;
        return;
    }
    if (_brightness == 0) {
        digitalWrite(BK_LIGHT_PIN, 1);
        _brightness = steps;
        delayMicroseconds(30);
    }
    int from = steps - _brightness;
    int to = steps - value;
    int num = (steps + to - from) % steps;
    for (int i = 0; i < num; i++) {
        digitalWrite(BK_LIGHT_PIN, 0);
        digitalWrite(BK_LIGHT_PIN, 1);
    }
    _brightness = value;
}

void setup() {
    // 1. Turn on system power rails
    pinMode(PWR_ON_PIN, OUTPUT);
    digitalWrite(PWR_ON_PIN, HIGH);

    pinMode(PWR_EN_PIN, OUTPUT);
    digitalWrite(PWR_EN_PIN, HIGH);

    // 2. Initialize buttons
    pinMode(BUTTON1_PIN, INPUT_PULLUP);
    pinMode(BUTTON2_PIN, INPUT_PULLUP);

    // 3. Serial logging
    Serial.begin(115200);
    delay(50);
    Serial.println("\n==================================");
    Serial.println("  LILYGO T-HMI MULTI-APP SYSTEM");
    Serial.println("==================================");

    // 4. Initialize Touch Digitizer for Portrait
    data_init();
    touch_calibration_t calData[4];
    bool hasNvsCal = data_read(calData);
    SPI.begin(TOUCHSCREEN_SCLK_PIN, TOUCHSCREEN_MISO_PIN, TOUCHSCREEN_MOSI_PIN);
    touch.begin(240, 320);
    apply_touch_calibration(touch, hasNvsCal ? calData : nullptr);

    // 5. Initialize Display in 240x320 Portrait
    tft.begin();
    tft.setRotation(0);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    // 6. Turn on Backlight to full brightness
    setBacklight(16);

    // 7. Register apps with OS Manager
    sysMonApp.setTouchController(&touch);
    appManager.registerApp(&drawingPadApp);
    appManager.registerApp(&arcadeApp);
    appManager.registerApp(&sysMonApp);

    // 8. Launch Home Screen
    appManager.begin();
    lastLoopTime = millis();
}

void loop() {
    unsigned long now = millis();
    unsigned long dt = now - lastLoopTime;
    lastLoopTime = now;

    appManager.update(dt);
    delay(2);
}
