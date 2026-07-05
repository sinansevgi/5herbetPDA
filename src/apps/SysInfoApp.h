#pragma once
#include "../App.h"
#include <vector>
#include <math.h>

class SysInfoApp : public App {
private:
    std::vector<int> ramHistory;
    std::vector<int> rssiHistory;
    unsigned long lastRamUpdate = 0;
    unsigned long lastRssiUpdate = 0;
    const int RAM_HIST_SIZE = 60;
    const int RSSI_HIST_SIZE = 120;

    void updateHistory() {
        unsigned long now = millis();
        // Update RAM history every 1000ms
        if (now - lastRamUpdate > 1000) {
            int totalKB = ESP.getHeapSize() / 1024;
            int freeKB = ESP.getFreeHeap() / 1024;
            int usedKB = totalKB - freeKB;
            ramHistory.push_back(usedKB);
            if (ramHistory.size() > RAM_HIST_SIZE) {
                ramHistory.erase(ramHistory.begin());
            }
            lastRamUpdate = now;
        }

        // Update RSSI history every 100ms
        if (now - lastRssiUpdate > 100) {
            int rssi = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : -100;
            rssiHistory.push_back(rssi);
            if (rssiHistory.size() > RSSI_HIST_SIZE) {
                rssiHistory.erase(rssiHistory.begin());
            }
            lastRssiUpdate = now;
        }
    }

    template <typename T>
    void drawDial(T& s, int cx, int cy, int radius, float value, float minVal, float maxVal, const char* label, const Theme& th, bool warning) {
        // Draw the outer ring
        s.drawCircle(cx, cy, radius, th.border);
        s.drawCircle(cx, cy, radius - 1, th.border);
        
        // Draw ticks
        for (int i = 0; i <= 100; i += 10) {
            float angle = PI * 0.75f + (i / 100.0f) * PI * 1.5f;
            int r1 = radius - 2;
            int r2 = (i % 50 == 0) ? radius - 8 : radius - 5;
            s.drawLine(cx + cos(angle) * r1, cy + sin(angle) * r1, cx + cos(angle) * r2, cy + sin(angle) * r2, th.border);
        }

        // Calculate needle angle
        float clampedVal = value;
        if (clampedVal < minVal) clampedVal = minVal;
        if (clampedVal > maxVal) clampedVal = maxVal;
        float normalized = (clampedVal - minVal) / (maxVal - minVal);
        float angle = PI * 0.75f + normalized * PI * 1.5f;

        // Draw needle
        uint16_t needleCol = warning ? th.danger : th.accent;
        s.drawLine(cx, cy, cx + cos(angle) * (radius - 10), cy + sin(angle) * (radius - 10), needleCol);
        s.fillCircle(cx, cy, 3, needleCol);

        // Label
        s.setTextColor(th.textDim);
        s.setTextSize(1);
        int textW = s.textWidth(label);
        s.setCursor(cx - textW / 2, cy + radius + 4);
        s.print(label);
        
        // Value Text
        s.setTextColor(needleCol);
        s.setTextSize(2);
        String valStr = String((int)value);
        int valW = s.textWidth(valStr.c_str());
        s.setCursor(cx - valW / 2, cy + radius / 2 - 4);
        s.print(valStr);
    }

public:
    void draw(AppContext* ctx) override {
        updateHistory();
        auto& s = *ctx->extSprite;
        const Theme& th = *ctx->theme;
        s.fillScreen(th.bg);

        // Header
        { char tb[8]; ctx->getTime(tb, sizeof(tb)); UI::drawHeader(s, th, ctx->userName.c_str(), tb); }

        int totalKB = ESP.getHeapSize() / 1024;
        
        // ---- 1. RAM History Chart ----
        int chartX = 8, chartY = 28, chartW = 304, chartH = 80;
        UI::drawRaisedPanel(s, chartX, chartY, chartW, chartH, th);
        s.setTextColor(th.textFaint);
        s.setCursor(chartX + 6, chartY + 6); s.print("RAM ALLOCATION (60s)");
        
        // Draw Chart Grid
        for(int i = 1; i < 4; i++) {
            s.drawFastHLine(chartX + 4, chartY + chartH - (i * chartH / 4), chartW - 8, th.border);
        }

        // Draw RAM History Line
        if (ramHistory.size() > 1) {
            float xStep = (float)(chartW - 8) / (RAM_HIST_SIZE - 1);
            for (size_t i = 1; i < ramHistory.size(); i++) {
                int px = chartX + 4 + (i - 1) * xStep;
                int x = chartX + 4 + i * xStep;
                
                int ph = (ramHistory[i - 1] * (chartH - 20)) / totalKB;
                int h = (ramHistory[i] * (chartH - 20)) / totalKB;
                
                int py = chartY + chartH - 4 - ph;
                int y = chartY + chartH - 4 - h;
                
                s.drawLine(px, py, x, y, th.accent);
                s.drawLine(px, py+1, x, y+1, th.accent); // Thicker line
            }
        }

        // ---- 2. Daemons Panel ----
        int daemonsX = 8, daemonsY = 116, daemonsW = 180, daemonsH = 104;
        UI::drawRaisedPanel(s, daemonsX, daemonsY, daemonsW, daemonsH, th);
        s.setTextColor(th.textFaint);
        s.setCursor(daemonsX + 6, daemonsY + 6); s.print("SYSTEM DAEMONS");

        auto drawDaemon = [&](int y_offset, const char* name, int state) {
            s.setTextColor(th.textDim);
            s.setCursor(daemonsX + 12, daemonsY + y_offset); s.print(name);
            uint16_t col;
            const char* stateStr;
            if (state == 1) { col = th.indicator; stateStr = "ACTIVE"; }
            else if (state == 0) { col = th.textFaint; stateStr = "IDLE"; }
            else { col = th.danger; stateStr = "ERROR"; }
            
            s.fillRoundRect(daemonsX + 120, daemonsY + y_offset - 2, 48, 12, 2, col);
            s.setTextColor(th.bg);
            s.setCursor(daemonsX + 124, daemonsY + y_offset); s.print(stateStr);
        };

        int wifiState = (WiFi.status() == WL_CONNECTED) ? 1 : 0;
        drawDaemon(24, "WiFi Client", wifiState);
        drawDaemon(40, "NTP Sync", ctx->timesynced ? 1 : (wifiState ? 0 : -1));
        drawDaemon(56, "Web Server", 0); // Placeholder state
        drawDaemon(72, "SD Handler", ctx->sdAvailable ? 1 : -1);

        // ---- 3. SD Card Storage Visualizer ----
        int sdX = 196, sdY = 116, sdW = 116, sdH = 104;
        UI::drawRaisedPanel(s, sdX, sdY, sdW, sdH, th);
        s.setTextColor(th.textFaint);
        s.setCursor(sdX + 6, sdY + 6); s.print("STORAGE");

        if (ctx->sdAvailable) {
            uint64_t cardSize = SD.cardSize();
            uint64_t usedBytes = SD.usedBytes();
            float usedPct = (float)usedBytes / cardSize;
            
            int cx = sdX + sdW / 2;
            int cy = sdY + 48;
            int r = 26;
            
            s.drawCircle(cx, cy, r, th.border);
            s.drawCircle(cx, cy, r+1, th.border);
            // Draw pie/arc representation using lines
            int endAngle = 360 * usedPct;
            for(int a=0; a<endAngle; a++) {
                float rad = (a - 90) * PI / 180.0;
                s.drawLine(cx, cy, cx + cos(rad)*r, cy + sin(rad)*r, th.accent);
            }
            s.setTextColor(th.textDim);
            s.setTextSize(1);
            int tw = s.textWidth("USED");
            s.setCursor(cx - tw/2, cy - 4);
            s.print("USED");

            s.setCursor(sdX + 12, sdY + sdH - 16);
            s.printf("%lluMB / %lluMB", usedBytes/(1024*1024), cardSize/(1024*1024));
        } else {
            s.setTextColor(th.danger);
            s.setCursor(sdX + 24, sdY + 48); s.print("NO MEDIA");
        }

        // Footer
        UI::drawFooter(s, th, "ESC Back");
    }

    void drawStatus(AppContext* ctx) override {
        updateHistory();
        auto& is = *ctx->intDisplay;
        const Theme& th = *ctx->theme;
        is.fillScreen(th.bg);

        int cx1 = 60;
        int cy1 = 50;
        int cx2 = 180;
        int cy2 = 50;
        int radius = 35;

        // ---- Left Dial: Temperature ----
        float tempC = temperatureRead();
        bool tempWarn = tempC > 70.0f;
        drawDial(is, cx1, cy1, radius, tempC, 20.0f, 100.0f, "TEMP C", th, tempWarn);

        // ---- Right Dial: Memory ----
        int totalKB = ESP.getHeapSize() / 1024;
        int freeKB = ESP.getFreeHeap() / 1024;
        int usedKB = totalKB - freeKB;
        float memPct = (float)usedKB / totalKB * 100.0f;
        bool memWarn = freeKB < 20; // Warn if < 20KB free
        drawDial(is, cx2, cy2, radius, memPct, 0.0f, 100.0f, "MEM %", th, memWarn);

        // ---- RSSI Oscilloscope ----
        int scopeX = 10, scopeY = 110, scopeW = 220, scopeH = 20;
        is.drawRect(scopeX, scopeY, scopeW, scopeH, th.border);
        is.setTextColor(th.textFaint);
        is.setCursor(scopeX + 2, scopeY + 2); is.print("RSSI");

        if (rssiHistory.size() > 1) {
            float xStep = (float)(scopeW - 30) / (RSSI_HIST_SIZE - 1);
            for (size_t i = 1; i < rssiHistory.size(); i++) {
                int px = scopeX + 28 + (i - 1) * xStep;
                int x = scopeX + 28 + i * xStep;
                
                // RSSI is typically -100 (bad) to -30 (good)
                auto mapRssi = [](int r, int h) {
                    if (r < -100) r = -100;
                    if (r > -30) r = -30;
                    float n = (r + 100) / 70.0f; // 0.0 to 1.0
                    return (int)(h - (n * h));
                };

                int py = scopeY + mapRssi(rssiHistory[i - 1], scopeH - 2);
                int y = scopeY + mapRssi(rssiHistory[i], scopeH - 2);
                
                is.drawLine(px, py, x, y, th.indicator);
            }
        }

        // ---- System Alert Overlay ----
        if (memWarn) {
            is.fillRect(0, 0, 240, 20, th.danger);
            is.setTextColor(th.bg);
            is.setTextSize(2);
            int tw = is.textWidth("LOW MEMORY");
            is.setCursor(120 - tw/2, 2);
            is.print("LOW MEMORY");
        }
    }

    bool handleInput(AppContext* ctx, char key, bool isDel, bool isEnter) override {
        return false;
    }
};
