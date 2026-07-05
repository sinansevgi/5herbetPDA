#pragma once
#include "../App.h"
#include <SD.h>
#include <ArduinoJson.h>

class DesktopApp : public App {
private:
    int selectedIndex = 0;
    unsigned long lastKeyboardActivity = 0;

    struct AppEntry {
        const char* name;
        const char* label;   // Short label for grid cell
        AppType type;
        uint16_t pastelColor;
    };

    static const int TOTAL_APPS = 9;
    AppEntry apps[TOTAL_APPS] = {
        { "Word",      "WORD",  AppType::NOTEPAD,      0xD472 }, // Dusty Rose
        { "Calc",      "CALC",  AppType::CALCULATOR,   0xACD8 }, // Muted Lavender
        { "Calendar",  "CAL",   AppType::CALENDAR,     0x9D73 }, // Soft Sage
        { "System",    "SYS",   AppType::SYSINFO,      0xE531 }, // Dusty Peach
        { "Files",     "FILE",  AppType::FILEEXPLORER, 0x9539 }, // Muted Periwinkle
        { "RSS",       "RSS",   AppType::RSSREADER,    0xDC0F }, // Dusty Coral
        { "Settings",  "SET",   AppType::SETTINGS,     0xCD59 }, // Soft Lilac
        { "Atlas",     "MAP",   AppType::ATLAS,        0x8DE5 }, // Muted Mint
        { "Utilities", "UTIL",  AppType::UTILITIES,    0xD5D2 }, // Sand/Beige
    };

    // ====================================================================
    // ICON FAMILY — consistent 1px stroke, industrial/technical style
    // All icons drawn in a sz×sz area at (x, y)
    // Inspired by icons printed on vintage electronic equipment
    // ====================================================================

    void iconNote(M5Canvas& s, int x, int y, int sz, uint16_t c) {
        int m = 4, w = sz - 2*m, h = sz - 2*m;
        s.drawRoundRect(x+m, y+m, w, h, 2, c);
        
        int headerH = h / 4;
        if (headerH < 4) headerH = 4;
        s.drawLine(x+m, y+m+headerH, x+m+w-1, y+m+headerH, c);
        
        int startY = y + m + headerH + 3;
        int availH = (y + m + h - 2) - startY;
        int lineSpacing = 3;
        if (availH > 8) lineSpacing = 4;
        
        int numLines = availH / lineSpacing;
        if (numLines > 4) numLines = 4;
        
        for (int i = 0; i < numLines; i++) {
            int ly = startY + i * lineSpacing;
            s.drawLine(x+m+3, ly, x+m+w-3, ly, c);
        }
    }

    void iconCalc(M5Canvas& s, int x, int y, int sz, uint16_t c) {
        int m = 4, w = sz - 2*m, h = sz - 2*m;
        s.drawRoundRect(x+m, y+m, w, h, 2, c);
        
        int dispH = h / 4;
        if (dispH < 5) dispH = 5;
        s.drawRect(x+m+3, y+m+3, w-6, dispH, c);
        
        int btnAreaY = y + m + dispH + 4;
        int btnAreaH = (y + m + h - 3) - btnAreaY;
        
        int rows = 2;
        int cols = 3;
        
        int btnW = (w - 6 - (cols - 1) * 2) / cols;
        int btnH = (btnAreaH - (rows - 1) * 2) / rows;
        
        for (int r = 0; r < rows; r++) {
            for (int col = 0; col < cols; col++) {
                int bx = x + m + 3 + col * (btnW + 2);
                int by = btnAreaY + r * (btnH + 2);
                s.fillRect(bx, by, btnW, btnH, c);
            }
        }
    }

    void iconCal(M5Canvas& s, int x, int y, int sz, uint16_t c) {
        int m = 4, w = sz - 2*m, h = sz - 2*m;
        s.drawRoundRect(x+m, y+m+3, w, h-3, 2, c);
        
        s.fillRect(x+m+3, y+m, 2, 4, c);
        s.fillRect(x+m+w-5, y+m, 2, 4, c);
        
        int headerH = (h - 3) / 4;
        if (headerH < 4) headerH = 4;
        s.drawLine(x+m, y+m+3+headerH, x+m+w-1, y+m+3+headerH, c);
        
        int gridY = y + m + 3 + headerH + 2;
        int gridH = (y + m + h - 2) - gridY;
        
        int rows = 2;
        int cols = 3;
        
        int btnW = (w - 6 - (cols - 1) * 2) / cols;
        int btnH = (gridH - (rows - 1) * 2) / rows;
        
        for (int r = 0; r < rows; r++) {
            for (int col = 0; col < cols; col++) {
                int bx = x + m + 3 + col * (btnW + 2);
                int by = gridY + r * (btnH + 2);
                s.fillRect(bx, by, btnW, btnH, c);
            }
        }
    }

    void iconSys(M5Canvas& s, int x, int y, int sz, uint16_t c) {
        int cx = x + sz / 2;
        int cy = y + sz / 2;
        int r = sz / 2 - 5;
        
        s.drawRoundRect(cx - r, cy - r, r * 2, r * 2, 2, c);
        
        int coreSz = r - 2;
        if (coreSz > 2) {
            s.fillRect(cx - coreSz, cy - coreSz, coreSz * 2, coreSz * 2, c);
        }
        
        int pinCount = 3;
        int pinSpacing = r * 2 / (pinCount + 1);
        if (pinSpacing < 3) pinSpacing = 3;
        
        for (int i = 0; i < pinCount; i++) {
            int offset = -r + (i + 1) * pinSpacing;
            s.drawLine(cx + offset, cy - r - 2, cx + offset, cy - r, c);
            s.drawLine(cx + offset, cy + r, cx + offset, cy + r + 2, c);
            s.drawLine(cx - r - 2, cy + offset, cx - r, cy + offset, c);
            s.drawLine(cx + r, cy + offset, cx + r + 2, cy + offset, c);
        }
    }

    void iconFile(M5Canvas& s, int x, int y, int sz, uint16_t c) {
        int m = 4, w = sz - 2*m, h = sz - 2*m;
        
        int tabW = w / 2;
        int tabH = h / 5;
        if (tabH < 3) tabH = 3;
        
        s.drawLine(x+m+2, y+m, x+m+2+tabW, y+m, c);
        s.drawLine(x+m+2, y+m, x+m+2, y+m+tabH, c);
        s.drawLine(x+m+2+tabW, y+m, x+m+2+tabW, y+m+tabH, c);
        
        s.drawRoundRect(x+m, y+m+tabH, w, h-tabH, 2, c);
        
        int lineY = y + m + tabH + 4;
        int availH = (y + m + h - 2) - lineY;
        int spacing = 3;
        if (availH > 8) spacing = 4;
        
        int numLines = availH / spacing;
        if (numLines > 3) numLines = 3;
        
        for (int i = 0; i < numLines; i++) {
            s.drawLine(x+m+4, lineY + i*spacing, x+m+w-4, lineY + i*spacing, c);
        }
    }

    void iconRss(M5Canvas& s, int x, int y, int sz, uint16_t c) {
        int cx = x + 5;
        int cy = y + sz - 5;
        
        s.fillCircle(cx, cy, 2, c);
        
        int step = sz / 4;
        if (step < 5) step = 5;
        
        for (int i = 1; i <= 2; i++) {
            int r = i * step;
            s.drawArc(cx, cy, r, r + 1, 270, 360, c);
        }
    }

    void iconSettings(M5Canvas& s, int x, int y, int sz, uint16_t c) {
        int cx = x + sz / 2;
        int cy = y + sz / 2;
        int w = sz - 8;
        int h = sz - 8;
        
        int spacing = h / 3;
        if (spacing < 5) spacing = 5;
        
        int startY = cy - spacing;
        
        for (int i = 0; i < 3; i++) {
            int ly = startY + i * spacing;
            s.drawLine(cx - w/2, ly, cx + w/2, ly, c);
            
            int handleX = cx;
            if (i == 0) handleX = cx - w/4;
            else if (i == 2) handleX = cx + w/4;
            
            s.fillRect(handleX - 2, ly - 3, 4, 6, c);
        }
    }

    void iconAtlas(M5Canvas& s, int x, int y, int sz, uint16_t c) {
        int cx = x + sz / 2;
        int cy = y + sz / 2;
        int r = sz / 2 - 3;
        
        s.drawCircle(cx, cy, r, c);
        s.drawLine(cx - r, cy, cx + r, cy, c);
        s.drawEllipse(cx, cy, r / 2, r, c);
        
        int py = r / 2;
        s.drawLine(cx - r + 2, cy - py, cx + r - 2, cy - py, c);
        s.drawLine(cx - r + 2, cy + py, cx + r - 2, cy + py, c);
    }

    void iconEmpty(M5Canvas& s, int x, int y, int sz, uint16_t c) {
        int m = 6;
        s.drawRoundRect(x+m, y+m, sz-2*m, sz-2*m, 3, c);
        s.drawLine(x+m, y+m, x+sz-m, y+sz-m, c);
        s.drawLine(x+sz-m, y+m, x+m, y+sz-m, c);
    }

    void iconUtils(M5Canvas& s, int x, int y, int sz, uint16_t c) {
        int cx = x + sz / 2;
        int cy = y + sz / 2 + 1;
        int r = sz / 2 - 4;
        
        s.drawCircle(cx, cy, r, c);
        s.drawRect(cx - 2, y + 1, 4, 3, c);
        
        s.drawLine(cx, cy, cx, cy - r + 3, c);
        s.drawLine(cx, cy, cx + r/2, cy + 1, c);
    }

    void drawIcon(M5Canvas& s, int x, int y, int sz, AppType t, uint16_t c) {
        switch (t) {
            case AppType::NOTEPAD:      iconNote(s, x, y, sz, c);     break;
            case AppType::CALCULATOR:   iconCalc(s, x, y, sz, c);     break;
            case AppType::CALENDAR:     iconCal(s, x, y, sz, c);      break;
            case AppType::SYSINFO:      iconSys(s, x, y, sz, c);      break;
            case AppType::FILEEXPLORER: iconFile(s, x, y, sz, c);     break;
            case AppType::RSSREADER:    iconRss(s, x, y, sz, c);      break;
            case AppType::SETTINGS:     iconSettings(s, x, y, sz, c); break;
            case AppType::ATLAS:        iconAtlas(s, x, y, sz, c);    break;
            case AppType::UTILITIES:    iconUtils(s, x, y, sz, c);    break;
            default:                    iconEmpty(s, x, y, sz, c);    break;
        }
    }

    // ====================================================================
    // 5herbet PDA Logo — drawn programmatically
    // Rendered as styled text block with decorative frame
    // ====================================================================
    int getActiveTasksCount(AppContext* ctx) {
        if (ctx->sdAvailable && SD.exists("/5herbetPDA/organizer/data.json")) {
            File file = SD.open("/5herbetPDA/organizer/data.json", FILE_READ);
            if (file) {
                JsonDocument doc;
                DeserializationError err = deserializeJson(doc, file);
                file.close();
                if (!err) {
                    int activeCount = 0;
                    JsonArray tArr = doc["tasks"].as<JsonArray>();
                    for (JsonObject obj : tArr) {
                        bool completed = obj["completed"] | false;
                        if (!completed) {
                            activeCount++;
                        }
                    }
                    return activeCount;
                }
            }
        }
        return 2; // Fallback to mock count
    }

    int getUpcomingEventsCount(AppContext* ctx) {
        char curDateStr[32] = "2026-07-05"; // Default mock date matching local time
        if (ctx->timesynced) {
            unsigned long nowEpoch = ctx->epochBase + (millis() - ctx->epochMillis) / 1000UL;
            time_t localEpoch = (time_t)(nowEpoch + (long)ctx->timezoneOffsetHours * 3600L);
            struct tm* ti = gmtime(&localEpoch);
            if (ti) {
                snprintf(curDateStr, sizeof(curDateStr), "%04d-%02d-%02d", ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday);
            }
        }
        
        if (ctx->sdAvailable && SD.exists("/5herbetPDA/organizer/data.json")) {
            File file = SD.open("/5herbetPDA/organizer/data.json", FILE_READ);
            if (file) {
                JsonDocument doc;
                DeserializationError err = deserializeJson(doc, file);
                file.close();
                if (!err) {
                    int eventCount = 0;
                    JsonArray eArr = doc["events"].as<JsonArray>();
                    for (JsonObject obj : eArr) {
                        String eDate = obj["date"].as<String>();
                        if (eDate >= curDateStr) {
                            eventCount++;
                        }
                    }
                    return eventCount;
                }
            }
        }
        return 1; // Fallback mock upcoming event count
    }

public:
    void setup(AppContext* ctx, String args) override {}

    void draw(AppContext* ctx) override {
        auto& s = *ctx->extSprite;
        const Theme& th = *ctx->theme;
        char timeStr[8]; ctx->getTime(timeStr, sizeof(timeStr));

        // ---- Clear background ----
        s.fillScreen(th.bg);

        // ---- Top status bar (20px) ----
        UI::drawHeader(s, th, ctx->userName.c_str(), timeStr);

        // ---- Vertical separator between left and right panes ----
        s.drawLine(208, 20, 208, 220, th.border);

        // ====================================================================
        // LEFT PANE: 3×3 Application Grid
        // ====================================================================
        // LEFT PANE: 3×3 Application Grid
        // ====================================================================
        const int COLS = 3;
        const int cellW = 66;
        const int cellH = 56;
        const int startX = 4;
        const int gapX = 3;
        const int startY = 28;
        const int gapY = 10;

        for (int i = 0; i < TOTAL_APPS; i++) {
            int row = i / COLS;
            int col = i % COLS;
            int cx = startX + col * (cellW + gapX);
            int cy = startY + row * (cellH + gapY);

            bool sel = (i == selectedIndex);
            bool usePastel = (th.interactionStyle == InteractionStyle::PASTEL);

            int lw = strlen(apps[i].label) * 6;
            int ix, iy, sz;

            if (sel) {
                // Micro-animation: shift up and scale
                sz = 30;
                ix = cx + (cellW - sz) / 2;
                iy = cy + 4; // Shifted up
            } else {
                sz = 26;
                ix = cx + (cellW - sz) / 2;
                iy = cy + 8; // Centered
            }

            if (usePastel) {
                if (sel) {
                    s.fillRoundRect(cx, cy, cellW, cellH, 8, th.selectBg);
                    drawIcon(s, ix, iy, sz, apps[i].type, apps[i].pastelColor);
                    s.setTextColor(th.textPrimary);
                    s.setTextSize(1);
                    s.setCursor(cx + (cellW - lw)/2, cy + 42);
                    s.print(apps[i].label);
                } else {
                    drawIcon(s, ix, iy, sz, apps[i].type, apps[i].pastelColor);
                    s.setTextColor(th.textDim);
                    s.setTextSize(1);
                    s.setCursor(cx + (cellW - lw)/2, cy + 42);
                    s.print(apps[i].label);
                }
            } else {
                if (sel) {
                    UI::drawRaisedPanel(s, cx + 1, cy + 1, cellW - 2, cellH - 2, th);
                    s.drawRect(cx, cy, cellW, cellH, th.accent);
                    s.fillRect(cx + 1, cy + cellH/2 - 4, 3, 8, th.accent);

                    drawIcon(s, ix, iy, sz, apps[i].type, th.accentBright);

                    s.setTextColor(th.accent);
                    s.setTextSize(1);
                    s.setCursor(cx + (cellW - lw)/2, cy + 42);
                    s.print(apps[i].label);
                } else {
                    s.drawRect(cx, cy, cellW, cellH, th.border);

                    drawIcon(s, ix, iy, sz, apps[i].type, th.steel);

                    s.setTextColor(th.steel);
                    s.setTextSize(1);
                    s.setCursor(cx + (cellW - lw)/2, cy + 42);
                    s.print(apps[i].label);
                }
            }
        }

        // ====================================================================
        // RIGHT PANE: "Today" Overview Sidebar
        // ====================================================================
        // 1. Date Box (y=28..78)
        s.drawRoundRect(212, 28, 102, 50, 4, th.border);
        char dayOfWeekStr[8] = "SUN";
        char monthDayStr[12] = "05 JUL";
        if (ctx->timesynced) {
            unsigned long nowEpoch = ctx->epochBase + (millis() - ctx->epochMillis) / 1000UL;
            time_t localEpoch = (time_t)(nowEpoch + (long)ctx->timezoneOffsetHours * 3600L);
            struct tm* ti = gmtime(&localEpoch);
            if (ti) {
                const char* wdays[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
                const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
                snprintf(dayOfWeekStr, sizeof(dayOfWeekStr), "%s", wdays[ti->tm_wday]);
                snprintf(monthDayStr, sizeof(monthDayStr), "%02d %s", ti->tm_mday, months[ti->tm_mon]);
            }
        }
        s.setTextColor(th.accentDim);
        s.setTextSize(1);
        s.setCursor(218, 34);
        s.print("TODAY");
        s.setTextColor(th.accentBright);
        s.setTextSize(2);
        s.setCursor(218, 44);
        s.print(dayOfWeekStr);
        s.setTextColor(th.textPrimary);
        s.setTextSize(1);
        s.setCursor(218, 64);
        s.print(monthDayStr);

        // 2. Organizer Box (y=86..136)
        s.drawRoundRect(212, 86, 102, 50, 4, th.border);
        s.setTextColor(th.accentDim);
        s.setTextSize(1);
        s.setCursor(218, 92);
        s.print("TASKS");
        int activeTasks = getActiveTasksCount(ctx);
        s.setTextColor(th.accentBright);
        s.setTextSize(2);
        s.setCursor(218, 102);
        s.print(activeTasks);
        int xOffset = (activeTasks >= 10) ? 242 : 236;
        s.setTextColor(th.textDim);
        s.setTextSize(1);
        s.setCursor(xOffset, 108);
        s.print("Active Tasks");

        // 3. Alarms & Events Box (y=144..214)
        s.drawRoundRect(212, 144, 102, 70, 4, th.border);
        s.setTextColor(th.accentDim);
        s.setTextSize(1);
        s.setCursor(218, 150);
        s.print("ALARM");
        String alarmStr = "No Alarms";
        for (const auto& al : ctx->alarms) {
            if (al.enabled) {
                char abuf[16];
                snprintf(abuf, sizeof(abuf), "%02d:%02d", al.hour, al.minute);
                alarmStr = abuf;
                break;
            }
        }
        s.setTextColor(th.textPrimary);
        s.setTextSize(1);
        s.setCursor(218, 160);
        s.print(alarmStr);

        s.drawLine(216, 172, 310, 172, th.border);
        s.setTextColor(th.accentDim);
        s.setTextSize(1);
        s.setCursor(218, 176);
        s.print("EVENTS");
        int upcomingEvents = getUpcomingEventsCount(ctx);
        s.setTextColor(th.accentBright);
        s.setTextSize(2);
        s.setCursor(218, 186);
        s.print(upcomingEvents);
        int evXOffset = (upcomingEvents >= 10) ? 242 : 236;
        s.setTextColor(th.textDim);
        s.setTextSize(1);
        s.setCursor(evXOffset, 192);
        s.print("Upcoming");

        // ---- Footer ----
        char footerBuf[64];
        snprintf(footerBuf, sizeof(footerBuf), "[%s]  WASD:Nav  ENTER:Open  ESC:Home", apps[selectedIndex].name);
        UI::drawFooter(s, th, footerBuf);

        // ---- Decorative: firmware version in bottom-right of footer ----
        s.setTextColor(th.textFaint);
        s.setTextSize(1);
        s.setCursor(280, 230);
        s.print("FW 1.2");
    }

    void drawStatus(AppContext* ctx) override {
        auto& is = *ctx->intDisplay;
        const Theme& th = *ctx->theme;
        char timeStr[8]; ctx->getTime(timeStr, sizeof(timeStr));

        // 1. Center branding logo panel
        int logoX = 60;
        int logoY = 8;
        int logoW = 120;
        int logoH = 26;
        is.drawRect(logoX, logoY, logoW, logoH, th.border);
        is.drawRect(logoX + 2, logoY + 2, logoW - 4, logoH - 4, th.accent);

        is.setTextColor(th.accentBright);
        is.setTextSize(2);
        is.setCursor(logoX + 10, logoY + 5);
        is.print("5herbet");

        is.setTextColor(th.steel);
        is.setTextSize(1);
        is.setCursor(logoX + 96, logoY + 13);
        is.print("PDA");

        // 2. Large clock & date below logo (left column)
        is.setTextColor(th.textPrimary);
        is.setTextSize(3);
        is.setCursor(15, 48);
        is.print(timeStr);

        char dateBuf[8]; ctx->getDateDDMM(dateBuf, sizeof(dateBuf));
        char yearBuf[6]; ctx->getYear(yearBuf, sizeof(yearBuf));
        is.setTextColor(th.textDim);
        is.setTextSize(1);
        is.setCursor(15, 78);
        is.printf("%s.%s", dateBuf, yearBuf);

        // Vertical separator line
        is.drawLine(125, 42, 125, 94, th.border);

        // 3. Hardware state widgets (right column)
        // Battery outline
        int bat = M5Cardputer.Power.getBatteryLevel();
        is.drawRect(135, 46, 18, 10, th.border);
        is.fillRect(153, 49, 2, 4, th.border);
        int fillW = (bat * 14) / 100;
        if (fillW < 0) fillW = 0;
        if (fillW > 14) fillW = 14;
        uint16_t batCol = (bat < 20) ? th.danger : th.accent;
        is.fillRect(137, 48, fillW, 6, batCol);
        is.setTextColor(th.textPrimary);
        is.setTextSize(1);
        is.setCursor(162, 47);
        is.printf("%d%%", bat);

        // WiFi strength (bars)
        bool wifiUp = (WiFi.status() == WL_CONNECTED);
        int32_t rssi = wifiUp ? WiFi.RSSI() : -100;
        int wifiLevel = 0;
        if (wifiUp) {
            if (rssi > -67) wifiLevel = 4;
            else if (rssi > -70) wifiLevel = 3;
            else if (rssi > -80) wifiLevel = 2;
            else wifiLevel = 1;
        }
        for (int b = 0; b < 4; b++) {
            uint16_t bCol = (wifiUp && wifiLevel > b) ? th.accent : th.bgRecessed;
            is.fillRect(135 + b * 5, 70 - b * 2, 3, 2 + b * 2, bCol);
        }
        is.setTextColor(wifiUp ? th.textPrimary : th.textDim);
        is.setTextSize(1);
        is.setCursor(162, 62);
        is.print(wifiUp ? "NET OK" : "NET --");

        // MicroSD outline
        uint16_t sdCol = ctx->sdAvailable ? th.indicator : th.border;
        is.drawRect(135, 78, 12, 14, sdCol);
        is.drawLine(135, 81, 138, 78, sdCol);
        is.drawPixel(136, 79, sdCol);
        if (ctx->sdAvailable) {
            is.fillRect(138, 80, 2, 2, sdCol);
            is.fillRect(142, 80, 2, 2, sdCol);
        }
        is.setTextColor(ctx->sdAvailable ? th.textPrimary : th.textDim);
        is.setTextSize(1);
        is.setCursor(162, 82);
        is.print(ctx->sdAvailable ? "SD OK" : "NO SD");

        // 4. Ambient / Keyboard-triggered Oscilloscope Pulse
        int centerY = 118;
        if (M5Cardputer.Keyboard.isPressed() || M5Cardputer.Speaker.isPlaying()) {
            lastKeyboardActivity = millis();
        }
        float amplitude = 2.0f;
        float speed = 0.005f;
        unsigned long timeDiff = millis() - lastKeyboardActivity;
        float actFactor = 0.0f;
        if (timeDiff < 1000) {
            actFactor = 1.0f - ((float)timeDiff / 1000.0f);
            amplitude = 2.0f + actFactor * 16.0f;
            speed = 0.005f + actFactor * 0.02f;
        }
        int prevY = 0;
        uint16_t pulseColor = (actFactor > 0.0f) ? th.accentBright : th.accentDim;
        for (int x = 10; x < 230; x++) {
            float angle = (x * 0.12f) - (millis() * speed * 100.0f);
            float yOffset = (sin(angle) * 0.7f + sin(angle * 2.3f) * 0.3f) * amplitude;
            if (x > 10) {
                is.drawLine(x - 1, centerY + prevY, x, centerY + (int)yOffset, pulseColor);
            }
            prevY = (int)yOffset;
        }
    }


    bool handleInput(AppContext* ctx, char key, bool isDel, bool isEnter) override {
        const int COLS = 3;
        const int ROWS = (TOTAL_APPS + COLS - 1) / COLS;

        int row = selectedIndex / COLS;
        int col = selectedIndex % COLS;

        if (key == 's' || key == '.') {
            row = (row + 1) % ROWS;
        } else if (key == 'w' || key == ';') {
            row = (row - 1 + ROWS) % ROWS;
        } else if (key == 'd' || key == '/') {
            col = (col + 1) % COLS;
        } else if (key == 'a' || key == ',') {
            col = (col - 1 + COLS) % COLS;
        } else if (isEnter) {
            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
            if (apps[selectedIndex].type != AppType::NONE) {
                ctx->requestAppSwitch(apps[selectedIndex].type);
            }
        }

        int newIdx = row * COLS + col;
        if (newIdx < TOTAL_APPS && newIdx != selectedIndex) {
            selectedIndex = newIdx;
            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
        }
        return false;
    }
};
