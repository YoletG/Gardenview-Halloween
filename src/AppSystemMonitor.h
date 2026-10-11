#pragma once
#include "App.h"
#include "pins.h"
#include "data.h"
#include <WiFi.h>

class AppSystemMonitor : public App {
private:
    enum ViewTab { TAB_SOC = 0, TAB_BATTERY, TAB_WIFI, TAB_SENSORS, NUM_TABS };
    ViewTab activeTab = TAB_SOC;
    const char* tabNames[NUM_TABS] = { "SoC", "Power", "Wi-Fi", "Sensors" };

    unsigned long lastRefreshTime = 0;
    const unsigned long REFRESH_INTERVAL = 500; // ms

    // WiFi Scanner State
    bool wifiScanning = false;
    int wifiCount = -1;

    // Calibration Mode
    bool calibrating = false;
    int calPoint = 0;
    touch_calibration_t calData[4];
    std::vector<touch_calibration_t> calSamples;
    unsigned long calLastTouch = 0;
    XPT2046* touchController = nullptr;

    void drawTabs(TFT_eSPI& tft) {
        tft.fillRect(0, 26, 240, 26, 0x18E3);
        int tabW = 240 / NUM_TABS; // 60px each
        for (int i = 0; i < NUM_TABS; i++) {
            int x = i * tabW;
            if (i == (int)activeTab) {
                tft.fillRect(x, 26, tabW, 24, 0x037A);
                tft.drawFastHLine(x, 49, tabW, TFT_CYAN);
                tft.setTextColor(TFT_WHITE, 0x037A);
            } else {
                tft.fillRect(x, 26, tabW, 24, 0x18E3);
                tft.setTextColor(0x94B2, 0x18E3);
            }
            tft.setTextDatum(MC_DATUM);
            tft.drawString(tabNames[i], x + tabW / 2, 38, 2);
        }
        tft.drawFastHLine(0, 51, 240, 0x3186);
    }

    void drawCardOutline(TFT_eSPI& tft, int x, int y, int w, int h, const char* title, uint16_t headerColor = 0x2145) {
        tft.fillRoundRect(x, y, w, h, 4, 0x1082);
        tft.drawRoundRect(x, y, w, h, 4, 0x39E7);
        tft.fillRoundRect(x, y, w, 18, 4, headerColor);
        tft.drawRoundRect(x, y, w, 18, 4, 0x39E7);
        tft.setTextColor(TFT_WHITE, headerColor);
        tft.setTextDatum(TL_DATUM);
        tft.drawString(title, x + 6, y + 3, 2);
    }

    void initSocTab(TFT_eSPI& tft) {
        tft.fillRect(0, 52, 240, 268, TFT_BLACK);

        drawCardOutline(tft, 6, 56, 228, 76, "ESP32-S3 Dual-Core", 0x1A2D);
        tft.setTextColor(0xCE59, 0x1082);
        tft.setTextDatum(TL_DATUM);
        char buf[32];
        snprintf(buf, sizeof(buf), "Clock Speed: %d MHz", getCpuFrequencyMhz());
        tft.drawString(buf, 14, 78, 2);

        drawCardOutline(tft, 6, 138, 228, 92, "Memory & Storage", 0x2A0F);
        tft.drawRect(14, 178, 200, 6, 0x39E7);

        drawCardOutline(tft, 6, 236, 228, 74, "Hardware Model", 0x122B);
        tft.setTextColor(0xCE59, 0x1082);
        tft.drawString("LilyGO T-HMI ESP32-S3", 14, 258, 2);
        tft.drawString("Display: 2.8\" ST7789 i80", 14, 274, 2);
        tft.drawString("Touch: XPT2046 12-bit SPI", 14, 290, 2);

        updateSocDynamic(tft);
    }

    void updateSocDynamic(TFT_eSPI& tft) {
        char buf[48];
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(0xCE59, 0x1082);

        float tempC = temperatureRead();
        float tempF = tempC * 1.8f + 32.0f;
        snprintf(buf, sizeof(buf), "Internal Temp: %.1f C (%.1f F)   ", tempC, tempF);
        tft.drawString(buf, 14, 94, 2);

        unsigned long secs = millis() / 1000;
        snprintf(buf, sizeof(buf), "Uptime: %luh %lum %lus      ", secs / 3600, (secs % 3600) / 60, secs % 60);
        tft.drawString(buf, 14, 110, 2);

        uint32_t freeHeap = ESP.getFreeHeap();
        uint32_t totalHeap = ESP.getHeapSize();
        uint32_t freePsram = ESP.getFreePsram();
        uint32_t totalPsram = ESP.getPsramSize();
        uint32_t flashSizeMb = ESP.getFlashChipSize() / (1024 * 1024);

        snprintf(buf, sizeof(buf), "SRAM Heap: %u / %u KB   ", (unsigned)(totalHeap - freeHeap) / 1024, (unsigned)totalHeap / 1024);
        tft.drawString(buf, 14, 160, 2);

        int heapPct = totalHeap > 0 ? ((totalHeap - freeHeap) * 100) / totalHeap : 0;
        int barW = constrain((heapPct * 198) / 100, 0, 198);
        tft.fillRect(15, 179, barW, 4, TFT_GREEN);
        tft.fillRect(15 + barW, 179, 198 - barW, 4, 0x1082);

        snprintf(buf, sizeof(buf), "PSRAM: %u / %u MB   ", (unsigned)(totalPsram - freePsram) / (1024 * 1024), (unsigned)totalPsram / (1024 * 1024));
        tft.drawString(buf, 14, 188, 2);

        snprintf(buf, sizeof(buf), "Flash Memory: %u MB QIO", (unsigned)flashSizeMb);
        tft.drawString(buf, 14, 206, 2);
    }

    void initBatteryTab(TFT_eSPI& tft) {
        tft.fillRect(0, 52, 240, 268, TFT_BLACK);

        drawCardOutline(tft, 6, 56, 228, 140, "Battery & Power Rail", 0x03E0);
        tft.drawRoundRect(14, 130, 180, 24, 4, TFT_WHITE);
        tft.fillRoundRect(195, 136, 6, 12, 2, TFT_WHITE);

        drawCardOutline(tft, 6, 204, 228, 106, "Backlight Display", 0x3186);
        tft.setTextColor(0xCE59, 0x1082);
        tft.drawString("Brightness Stepping (GPIO 38):", 14, 228, 2);

        tft.fillRoundRect(14, 252, 60, 32, 4, 0x2165);
        tft.drawRoundRect(14, 252, 60, 32, 4, TFT_WHITE);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, 0x2165);
        tft.drawString("Low", 44, 268, 2);

        tft.fillRoundRect(86, 252, 60, 32, 4, 0x2165);
        tft.drawRoundRect(86, 252, 60, 32, 4, TFT_WHITE);
        tft.drawString("Med", 116, 268, 2);

        tft.fillRoundRect(158, 252, 60, 32, 4, 0x0410);
        tft.drawRoundRect(158, 252, 60, 32, 4, TFT_CYAN);
        tft.setTextColor(TFT_CYAN, 0x0410);
        tft.drawString("Max", 188, 268, 2);

        updateBatteryDynamic(tft);
    }

    void updateBatteryDynamic(TFT_eSPI& tft) {
        uint32_t mv = analogReadMilliVolts(BAT_ADC_PIN) * 2;
        float volts = mv / 1000.0f;
        int pct = 0;
        if (volts >= 4.20f) pct = 100;
        else if (volts <= 3.30f) pct = 0;
        else pct = (int)((volts - 3.30f) / (4.20f - 3.30f) * 100.0f);

        char buf[48];
        tft.setTextDatum(TL_DATUM);
        snprintf(buf, sizeof(buf), "ADC Raw mV: %u mV   ", (unsigned)mv);
        tft.setTextColor(0xCE59, 0x1082);
        tft.drawString(buf, 14, 78, 2);

        snprintf(buf, sizeof(buf), "Voltage: %.2f V     ", volts);
        tft.setTextColor(TFT_WHITE, 0x1082);
        tft.drawString(buf, 14, 98, 4);

        uint16_t barCol = TFT_GREEN;
        if (pct < 20) barCol = TFT_RED;
        else if (pct < 50) barCol = TFT_YELLOW;

        int fillW = constrain((pct * 174) / 100, 0, 174);
        tft.fillRect(17, 133, fillW, 18, barCol);
        tft.fillRect(17 + fillW, 133, 174 - fillW, 18, 0x18E3);

        tft.setTextDatum(MC_DATUM);
        snprintf(buf, sizeof(buf), "%d%%", pct);
        tft.setTextColor(TFT_WHITE, barCol);
        tft.drawString(buf, 104, 142, 2);

        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(0x94B2, 0x1082);
        if (volts > 4.30f) {
            tft.drawString("Status: Powered via USB-C (5V)   ", 14, 166, 2);
        } else {
            tft.drawString("Status: Running on LiPo Battery ", 14, 166, 2);
        }
    }

    void initWifiTab(TFT_eSPI& tft) {
        tft.fillRect(0, 52, 240, 268, TFT_BLACK);
        drawWifiList(tft);
    }

    void drawWifiList(TFT_eSPI& tft) {
        tft.fillRoundRect(16, 58, 208, 32, 5, wifiScanning ? 0x6280 : 0x037A);
        tft.drawRoundRect(16, 58, 208, 32, 5, TFT_WHITE);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, wifiScanning ? 0x6280 : 0x037A);
        tft.drawString(wifiScanning ? "SCANNING 2.4GHz..." : "SCAN WI-FI NETWORKS", 120, 74, 2);

        if (wifiScanning) {
            tft.fillRect(10, 100, 220, 180, TFT_BLACK);
            tft.setTextColor(TFT_YELLOW, TFT_BLACK);
            tft.drawString("Scanning airwaves...", 120, 150, 2);
        } else if (wifiCount >= 0) {
            tft.fillRect(6, 94, 228, 20, TFT_BLACK);
            tft.setTextDatum(TL_DATUM);
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            char buf[32];
            snprintf(buf, sizeof(buf), "Found %d networks:", wifiCount);
            tft.drawString(buf, 10, 98, 2);

            int maxItems = min(wifiCount, 7);
            for (int i = 0; i < maxItems; i++) {
                int y = 118 + i * 26;
                tft.fillRoundRect(6, y, 228, 23, 3, 0x1082);
                tft.drawRoundRect(6, y, 228, 23, 3, 0x2965);

                String ssid = WiFi.SSID(i);
                if (ssid.length() == 0) ssid = "<Hidden>";
                if (ssid.length() > 16) ssid = ssid.substring(0, 15) + "..";

                tft.setTextColor(TFT_WHITE, 0x1082);
                tft.drawString(ssid.c_str(), 12, y + 4, 2);

                int32_t rssi = WiFi.RSSI(i);
                snprintf(buf, sizeof(buf), "%d dBm", (int)rssi);
                uint16_t sigCol = (rssi > -60) ? TFT_GREEN : (rssi > -75 ? TFT_YELLOW : TFT_RED);
                tft.setTextColor(sigCol, 0x1082);
                tft.setTextDatum(TR_DATUM);
                tft.drawString(buf, 226, y + 4, 2);
                tft.setTextDatum(TL_DATUM);
            }
        } else {
            tft.fillRect(10, 100, 220, 180, TFT_BLACK);
            tft.setTextDatum(MC_DATUM);
            tft.setTextColor(0x94B2, TFT_BLACK);
            tft.drawString("Tap SCAN to search nearby APs", 120, 150, 2);
        }
    }

    void initSensorsTab(TFT_eSPI& tft) {
        tft.fillRect(0, 52, 240, 268, TFT_BLACK);

        drawCardOutline(tft, 6, 56, 228, 76, "Magnetic Reed Switch", 0x3186);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(0xCE59, 0x1082);
        tft.drawString("Pin: GPIO 21 (Internal)", 14, 78, 2);

        drawCardOutline(tft, 6, 138, 228, 62, "Hardware BOOT Key", 0x18E3);
        tft.setTextColor(0xCE59, 0x1082);
        tft.drawString("Pin: GPIO 0 (Boot Key)", 14, 158, 2);

        drawCardOutline(tft, 6, 206, 228, 106, "Touchscreen & Calibration", 0x2124);

        // Calibrate Button
        tft.fillRoundRect(14, 276, 212, 28, 4, 0x9800);
        tft.drawRoundRect(14, 276, 212, 28, 4, TFT_YELLOW);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, 0x9800);
        tft.drawString("START TOUCH CALIBRATION", 120, 290, 2);

        updateSensorsDynamic(tft, TouchPoint());
    }

    void updateSensorsDynamic(TFT_eSPI& tft, const TouchPoint& touch) {
        bool reedClosed = (digitalRead(BUTTON2_PIN) == LOW);
        if (reedClosed) {
            tft.fillCircle(24, 110, 6, TFT_GREEN);
            tft.setTextColor(TFT_GREEN, 0x1082);
            tft.setTextDatum(TL_DATUM);
            tft.drawString("MAGNET DETECTED / CLOSED   ", 36, 103, 2);
        } else {
            tft.fillCircle(24, 110, 6, 0x52AA);
            tft.setTextColor(0x94B2, 0x1082);
            tft.setTextDatum(TL_DATUM);
            tft.drawString("No Magnetic Field (Open)   ", 36, 103, 2);
        }

        bool btnBoot = (digitalRead(BUTTON1_PIN) == LOW);
        if (btnBoot) {
            tft.fillCircle(24, 180, 6, TFT_YELLOW);
            tft.setTextColor(TFT_YELLOW, 0x1082);
            tft.drawString("BOOT KEY PRESSED   ", 36, 173, 2);
        } else {
            tft.fillCircle(24, 180, 6, 0x52AA);
            tft.setTextColor(0x94B2, 0x1082);
            tft.drawString("Key Released       ", 36, 173, 2);
        }

        char buf[48];
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(0xCE59, 0x1082);
        snprintf(buf, sizeof(buf), "State: %s  ", touch.pressed ? "PRESSED " : "RELEASED");
        tft.drawString(buf, 14, 226, 2);

        snprintf(buf, sizeof(buf), "Screen: X:%03d Y:%03d   ", touch.x, touch.y);
        tft.drawString(buf, 14, 242, 2);

        snprintf(buf, sizeof(buf), "Raw ADC: X:%04d Y:%04d ", touch.rawX, touch.rawY);
        tft.drawString(buf, 14, 258, 2);
    }

    void showCalTarget(TFT_eSPI& tft, int pt) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("TOUCH CALIBRATION", 120, 140, 2);
        char buf[32];
        snprintf(buf, sizeof(buf), "Touch crosshair %d of 4", pt + 1);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawString(buf, 120, 165, 2);

        int cx = 10, cy = 10;
        if (pt == 0) { cx = 10; cy = 10; }
        else if (pt == 1) { cx = 229; cy = 10; }
        else if (pt == 2) { cx = 229; cy = 309; }
        else if (pt == 3) { cx = 10; cy = 309; }

        tft.drawCircle(cx, cy, 10, TFT_RED);
        tft.drawCircle(cx, cy, 4, TFT_YELLOW);
        tft.drawFastHLine(max(0, cx - 14), cy, 29, TFT_WHITE);
        tft.drawFastVLine(cx, max(0, cy - 14), 29, TFT_WHITE);
    }

public:
    void setTouchController(XPT2046* tc) { touchController = tc; }

    const char* getName() const override { return "SysMonitor"; }
    const char* getDescription() const override { return "System health, power rail, sensors & Wi-Fi"; }
    uint16_t getThemeColor() const override { return TFT_BLUE; }

    void onEnter(TFT_eSPI& tft) override {
        calibrating = false;
        activeTab = TAB_SOC;
        wifiScanning = false;
        wifiCount = -1;
        drawTabs(tft);
        initSocTab(tft);
        lastRefreshTime = millis();
    }

    void onUpdate(TFT_eSPI& tft, const TouchPoint& touch, unsigned long dt) override {
        if (calibrating) {
            if (touch.pressed && touch.rawX > 50 && touch.rawY > 50) {
                calLastTouch = millis();
                calSamples.push_back((touch_calibration_t){ touch.rawX, touch.rawY });
            }
            if (!calSamples.empty() && millis() - calLastTouch > 300) {
                uint32_t sumX = 0, sumY = 0;
                for (auto& s : calSamples) {
                    sumX += s.rawX;
                    sumY += s.rawY;
                }
                calData[calPoint].rawX = sumX / calSamples.size();
                calData[calPoint].rawY = sumY / calSamples.size();
                calSamples.clear();

                calPoint++;
                if (calPoint >= 4) {
                    data_write(calData);
                    if (touchController) {
                        apply_touch_calibration(*touchController, calData);
                    }
                    tft.fillScreen(TFT_BLACK);
                    tft.setTextColor(TFT_GREEN, TFT_BLACK);
                    tft.setTextDatum(MC_DATUM);
                    tft.drawString("CALIBRATION SAVED!", 120, 160, 4);
                    delay(1200);
                    calibrating = false;
                    drawTabs(tft);
                    initSensorsTab(tft);
                    return;
                }
                showCalTarget(tft, calPoint);
            }
            return;
        }

        // Tab Switching
        if (touch.justPressed && touch.y >= 26 && touch.y <= 51) {
            int tabW = 240 / NUM_TABS;
            int clickedTab = touch.x / tabW;
            if (clickedTab >= 0 && clickedTab < NUM_TABS && clickedTab != (int)activeTab) {
                activeTab = (ViewTab)clickedTab;
                drawTabs(tft);
                if (activeTab == TAB_SOC) initSocTab(tft);
                else if (activeTab == TAB_BATTERY) initBatteryTab(tft);
                else if (activeTab == TAB_WIFI) initWifiTab(tft);
                else if (activeTab == TAB_SENSORS) initSensorsTab(tft);
                return;
            }
        }

        // Tab Actions
        if (activeTab == TAB_WIFI) {
            if (touch.justPressed && touch.x >= 16 && touch.x <= 224 && touch.y >= 58 && touch.y <= 90) {
                if (!wifiScanning) {
                    wifiScanning = true;
                    WiFi.mode(WIFI_STA);
                    WiFi.disconnect();
                    WiFi.scanNetworks(true);
                    drawWifiList(tft);
                }
            }
            if (wifiScanning) {
                int n = WiFi.scanComplete();
                if (n >= 0) {
                    wifiCount = n;
                    wifiScanning = false;
                    drawWifiList(tft);
                }
            }
        } else if (activeTab == TAB_SENSORS) {
            if (touch.justPressed && touch.x >= 14 && touch.x <= 226 && touch.y >= 276 && touch.y <= 306) {
                calibrating = true;
                calPoint = 0;
                calSamples.clear();
                showCalTarget(tft, calPoint);
                return;
            }
        }

        // Dynamic update without full screen flash
        if (millis() - lastRefreshTime >= REFRESH_INTERVAL) {
            lastRefreshTime = millis();
            if (activeTab == TAB_SOC) {
                updateSocDynamic(tft);
            } else if (activeTab == TAB_BATTERY) {
                updateBatteryDynamic(tft);
            } else if (activeTab == TAB_SENSORS) {
                updateSensorsDynamic(tft, touch);
            }
        }
    }

    void onExit(TFT_eSPI& tft) override {
        calibrating = false;
        if (wifiScanning) {
            WiFi.scanDelete();
            wifiScanning = false;
        }
    }
};
