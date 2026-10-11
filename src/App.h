#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <xpt2046.h>

struct TouchPoint {
    bool pressed;
    bool justPressed;
    bool justReleased;
    uint16_t x;
    uint16_t y;
    uint16_t rawX;
    uint16_t rawY;
};

class App {
public:
    virtual ~App() {}
    virtual const char* getName() const = 0;
    virtual const char* getDescription() const = 0;
    virtual uint16_t getThemeColor() const = 0;
    
    // Lifecycle hooks
    virtual void onEnter(TFT_eSPI& tft) = 0;
    virtual void onUpdate(TFT_eSPI& tft, const TouchPoint& touch, unsigned long dt) = 0;
    virtual void onExit(TFT_eSPI& tft) = 0;
};
