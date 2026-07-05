#pragma once
#include <M5Cardputer.h>
#include <M5GFX.h>

// =============================================================
// THEME SYSTEM
// =============================================================
enum class InteractionStyle {
    ORGANIZER,
    STEEL,
    VAPORWARE,
    HACKERPUNK,
    MATRIX,
    CASSETTE,
    PASTEL,
    PRIDE
};

struct Theme {
    char name[20];
    InteractionStyle interactionStyle;
    uint16_t bg;
    uint16_t bgPanel;
    uint16_t bgRecessed;
    uint16_t bgRaised;
    uint16_t accent;
    uint16_t accentDim;
    uint16_t accentBright;
    uint16_t textPrimary;
    uint16_t textDim;
    uint16_t textFaint;
    uint16_t steel;
    uint16_t indicator;     // teal / green status LED
    uint16_t indicatorDim;
    uint16_t danger;
    uint16_t selectBg;
    uint16_t border;
};

#include <vector>
#include "AudioSystem.h"
extern std::vector<Theme> ALL_THEMES;
extern const Theme THEME_FALLBACK;

// =============================================================
// UI DRAWING HELPERS
// =============================================================
namespace UI {

    // Standard header bar for all app screens (external 320×240)
    // Draws a 20px header with user name, branding, and time
    template <typename T>
    inline void drawHeader(T& s, const Theme& th, const char* userName, const char* timeStr) {
        // Background
        s.fillRect(0, 0, 320, 20, th.bgRaised);
        // Bottom border — thin double line
        s.drawLine(0, 20, 319, 20, th.border);
        s.drawLine(0, 21, 319, 21, th.bgRecessed);

        // Left: user name
        s.setTextSize(1);
        s.setTextColor(th.textDim);
        s.setCursor(4, 6);
        s.print(userName);

        // Center: 5herbet PDA
        s.setTextColor(th.accentDim);
        s.setCursor(122, 6);
        s.print("5herbet");

        // Right: time
        s.setTextColor(th.accent);
        int len = strlen(timeStr) * 6;
        s.setCursor(316 - len, 6);
        s.print(timeStr);
    }

    // Modern pill-styled badge string parser
    template <typename T>
    inline void drawBadgeString(T& s, int x, int y, const char* str, const Theme& th) {
        int cx = x;
        s.setTextSize(1);
        int len = strlen(str);
        for (int i = 0; i < len; ) {
            if (str[i] == '[') {
                int end = -1;
                for (int j = i + 1; j < len; j++) {
                    if (str[j] == ']') { end = j; break; }
                }
                if (end != -1) {
                    String badgeText = String(str).substring(i + 1, end);
                    int bw = badgeText.length() * 6 + 6;
                    s.fillRoundRect(cx, y - 2, bw, 11, 3, th.bgRecessed);
                    s.drawRoundRect(cx, y - 2, bw, 11, 3, th.accent);
                    s.setTextColor(th.accentBright);
                    s.setCursor(cx + 4, y);
                    s.print(badgeText.c_str());
                    cx += bw + 4;
                    i = end + 1;
                    continue;
                }
            }
            s.setTextColor(th.textFaint);
            s.setCursor(cx, y);
            s.print(str[i]);
            cx += 6;
            i++;
        }
    }

    // Standard footer bar (external 320×240)
    template <typename T>
    inline void drawFooter(T& s, const Theme& th, const char* hint) {
        // Top border
        s.drawLine(0, 224, 319, 224, th.border);
        s.fillRect(0, 225, 320, 15, th.bgRaised);
        drawBadgeString(s, 4, 229, hint, th);
    }

    // Thin recessed separator line
    template <typename T>
    inline void drawSeparator(T& s, int x, int y, int w, const Theme& th) {
        s.drawLine(x, y, x + w, y, th.bgRecessed);
        s.drawLine(x, y + 1, x + w, y + 1, th.border);
    }

    // Measurement ticks along edge (decorative)
    template <typename T>
    inline void drawTicks(T& s, int x, int y, int count, int spacing, bool vertical, uint16_t col) {
        for (int i = 0; i < count; i++) {
            if (vertical)
                s.drawLine(x, y + i * spacing, x + 2, y + i * spacing, col);
            else
                s.drawLine(x + i * spacing, y, x + i * spacing, y + 2, col);
        }
    }

    // Small LED indicator dot
    template <typename T>
    inline void drawLED(T& s, int x, int y, uint16_t col) {
        s.fillRect(x, y, 3, 3, col);
        s.drawPixel(x, y, 0x0000); // corner cut for diamond look
        s.drawPixel(x + 2, y, 0x0000);
        s.drawPixel(x, y + 2, 0x0000);
        s.drawPixel(x + 2, y + 2, 0x0000);
    }

    // Recessed panel (sunken area with border)
    template <typename T>
    inline void drawRecessedPanel(T& s, int x, int y, int w, int h, const Theme& th) {
        s.fillRect(x, y, w, h, th.bgRecessed);
        s.drawRect(x, y, w, h, th.border);
        // Top-left shadow edge
        s.drawLine(x, y, x + w - 1, y, th.bgRecessed);
        s.drawLine(x, y, x, y + h - 1, th.bgRecessed);
    }

    // Raised panel (elevated area)
    template <typename T>
    inline void drawRaisedPanel(T& s, int x, int y, int w, int h, const Theme& th) {
        s.fillRect(x, y, w, h, th.bgRaised);
        s.drawLine(x, y, x + w - 1, y, th.border);
        s.drawLine(x, y + h - 1, x + w - 1, y + h - 1, th.bgRecessed);
        s.drawLine(x + w - 1, y, x + w - 1, y + h - 1, th.bgRecessed);
    }

    // Section label — small uppercase text with underline
    template <typename T>
    inline void drawSectionLabel(T& s, int x, int y, const char* text, const Theme& th) {
        s.setTextSize(1);
        s.setTextColor(th.textFaint);
        s.setCursor(x, y);
        s.print(text);
        int len = strlen(text) * 6;
        s.drawLine(x, y + 10, x + len, y + 10, th.border);
    }

    // Startup Animation
    template <typename T1, typename T2>
    inline void drawStartupAnimation(T1& ext, T2& in, const Theme& th) {
        ext.fillScreen(th.bg);
        in.fillScreen(th.bg);
        ext.pushSprite(0, 0);
        
        if (th.interactionStyle == InteractionStyle::CASSETTE) {
            for (int i = 0; i < 4; i++) {
                ext.fillScreen(th.bg);
                in.fillScreen(th.bg);
                if (i % 2 == 0) {
                    ext.fillCircle(160, 120, 15, th.danger);
                    in.fillCircle(120, 67, 10, th.danger);
                }
                ext.pushSprite(0, 0);
                in.pushSprite(0, 0);
                SysAudio.update();
                delay(150);
            }
            ext.drawRect(10, 10, 300, 220, th.border);
            ext.pushSprite(0, 0);
            in.pushSprite(0, 0);
            delay(100);
        } else if (th.interactionStyle == InteractionStyle::HACKERPUNK) {
            const char* bootLines[] = {
                "init: fsociety firmware v1.0.4",
                "loading kernel...",
                "mounting rootfs... [OK]",
                "decrypting datastore...",
                "bypassing auth... [OK]",
                "root access granted."
            };
            ext.setTextColor(th.accent);
            ext.setTextSize(1);
            for (int i = 0; i < 6; i++) {
                ext.setCursor(10, 10 + i*16);
                ext.print(bootLines[i]);
                ext.pushSprite(0, 0);
                in.pushSprite(0, 0);
                SysAudio.update();
                delay(200);
            }
            delay(500);
        } else if (th.interactionStyle == InteractionStyle::MATRIX) {
            for (int j = 0; j < 30; j++) {
                for (int i = 0; i < 8; i++) {
                    int x = random(0, 320);
                    int y = random(0, 240);
                    ext.setTextColor(th.accent);
                    ext.setCursor(x, y);
                    ext.print(random(0, 2) ? "1" : "0");
                }
                ext.pushSprite(0, 0);
                in.pushSprite(0, 0);
                SysAudio.update();
                delay(20);
            }
        } else if (th.interactionStyle == InteractionStyle::VAPORWARE) {
            for (int y = 240; y > 120; y -= 4) {
                ext.fillScreen(th.bg);
                // Draw sun
                ext.fillCircle(160, 120, 40, th.accentBright);
                ext.fillRect(0, 120, 320, 120, th.bg);
                // Draw perspective grid
                for(int i=-160; i<480; i+=40) {
                    ext.drawLine(160, 120, i, 240, th.accent);
                }
                for(int h=240; h>120; h-=abs(240-h)/4 + 4) {
                    if (h > y) ext.drawLine(0, h, 320, h, th.accent);
                }
                ext.pushSprite(0, 0);
                in.pushSprite(0, 0);
                SysAudio.update();
                delay(20);
            }
        } else if (th.interactionStyle == InteractionStyle::STEEL) {
            for (int y = 0; y < 240; y += 8) {
                ext.fillScreen(th.bg);
                ext.drawLine(0, y, 320, y, th.accentBright);
                ext.fillRect(0, 0, 320, y, th.bgPanel);
                ext.pushSprite(0, 0);
                in.pushSprite(0, 0);
                SysAudio.update();
                delay(15);
            }
        } else if (th.interactionStyle == InteractionStyle::PASTEL) {
            for (int r = 0; r < 200; r += 8) {
                ext.fillCircle(160, 120, r, th.bgRaised);
                ext.drawCircle(160, 120, r, th.accentBright);
                ext.pushSprite(0, 0);
                in.pushSprite(0, 0);
                SysAudio.update();
                delay(15);
            }
        } else {
            ext.fillScreen(th.bg);
            ext.setTextColor(th.textPrimary);
            ext.setTextSize(3);
            ext.setCursor(60, 100);
            ext.print("5HERBET");
            ext.setTextSize(2);
            ext.setCursor(200, 108);
            ext.print("PDA");
            
            ext.drawRect(60, 150, 200, 10, th.border);
            for (int w = 0; w <= 200; w += 8) {
                ext.fillRect(60, 150, w, 10, th.accent);
                ext.pushSprite(0, 0);
                in.pushSprite(0, 0);
                SysAudio.update();
                delay(15);
            }
            delay(300);
        }
    }

    // Loading Animation
    template <typename T>
    inline void drawLoading(T& s, const Theme& th, int frame) {
        if (th.interactionStyle == InteractionStyle::CASSETTE) {
            int cx = 160, cy = 120, r = 16;
            s.drawCircle(cx, cy, r, th.border);
            float angle = frame * 0.5f;
            s.drawLine(cx, cy, cx + r * cos(angle), cy + r * sin(angle), th.accent);
            s.drawLine(cx, cy, cx + r * cos(angle + PI), cy + r * sin(angle + PI), th.accent);
        } else if (th.interactionStyle == InteractionStyle::MATRIX || th.interactionStyle == InteractionStyle::HACKERPUNK) {
            s.setTextColor(th.accent);
            s.setTextSize(2);
            s.setCursor(150, 110);
            const char* c = "|/-\\";
            s.print(c[(frame / 2) % 4]);
        } else {
            s.drawRect(150, 110, 20, 20, th.border);
            s.fillRect(152, 112 + ((frame / 2) % 16), 16, 4, th.accent);
        }
    }
}

// =============================================================
// APP TYPES
// =============================================================
enum class AppType {
    NONE,
    DESKTOP,
    NOTEPAD,
    CALCULATOR,
    CALENDAR,
    SYSINFO,
    FILEEXPLORER,
    SETTINGS,
    ATLAS,
    UTILITIES,
    RSSREADER,
    DICTIONARY
};

// =============================================================
// APP CONTEXT
// =============================================================
struct AlarmData {
    int hour;
    int minute;
    bool enabled;
    bool hasRungToday;
};

struct AppContext {
    M5Canvas* extSprite;
    LovyanGFX* intDisplay;
    bool sdAvailable;
    bool extScreenConnected = true;
    String notificationMsg;
    unsigned long notificationTime = 0;

    // Theme
    int themeIndex = 0;
    const Theme* theme = &THEME_FALLBACK;

    // User identity
    String userName = "HELLO STRANGER";

    // WiFi credentials
    String wifiSSID = "";
    String wifiPass = "";

    // Power settings
    int screenTimeoutMins = 5;
    int sleepTimeoutMins = 10;

    // Time / timezone
    int timezoneOffsetHours = 0;
    unsigned long epochBase = 0;
    unsigned long epochMillis = 0;
    bool timesynced = false;

    // Atlas / World Clock
    String pinnedWorldClockName = "";
    int pinnedWorldClockOffset = 0;

    // Alarms
    std::vector<AlarmData> alarms;
    bool alarmRinging = false;
    unsigned long alarmRingStart = 0;

    // App switching
    AppType nextApp = AppType::NONE;
    String nextAppArgs = "";
    bool hasUnsavedWork = false;

    // ---- Helpers ----
    void showNotification(String msg) {
        notificationMsg = msg;
        notificationTime = millis();
    }

    void requestAppSwitch(AppType app, String args = "") {
        if (hasUnsavedWork && app != AppType::NONE) {
            showNotification("Unsaved changes! Press key again.");
            hasUnsavedWork = false;
            return;
        }
        nextApp = app;
        nextAppArgs = args;
    }

    // Buffer-based time functions — no heap allocation
    void getTime(char* out, size_t len) const {
        unsigned long nowEpoch = epochBase + (millis() - epochMillis) / 1000UL;
        long localEpoch = (long)nowEpoch + (long)timezoneOffsetHours * 3600L;
        int h = ((localEpoch / 3600) % 24 + 24) % 24;
        int m = ((localEpoch / 60) % 60 + 60) % 60;
        snprintf(out, len, "%02d:%02d", h, m);
    }

    void getTimeFull(char* out, size_t len) const {
        unsigned long nowEpoch = epochBase + (millis() - epochMillis) / 1000UL;
        long localEpoch = (long)nowEpoch + (long)timezoneOffsetHours * 3600L;
        int h = ((localEpoch / 3600) % 24 + 24) % 24;
        int m = ((localEpoch / 60) % 60 + 60) % 60;
        int sec = ((localEpoch) % 60 + 60) % 60;
        snprintf(out, len, "%02d:%02d:%02d", h, m, sec);
    }

    void getDateDDMM(char* out, size_t len) const {
        unsigned long nowEpoch = epochBase + (millis() - epochMillis) / 1000UL;
        time_t localEpoch = (time_t)(nowEpoch + (long)timezoneOffsetHours * 3600L);
        struct tm* ti = gmtime(&localEpoch);
        if (ti && timesynced) {
            snprintf(out, len, "%02d.%02d", ti->tm_mday, ti->tm_mon + 1);
        } else {
            strncpy(out, "01.01", len);
        }
    }

    void getYear(char* out, size_t len) const {
        unsigned long nowEpoch = epochBase + (millis() - epochMillis) / 1000UL;
        time_t localEpoch = (time_t)(nowEpoch + (long)timezoneOffsetHours * 3600L);
        struct tm* ti = gmtime(&localEpoch);
        if (ti && timesynced) {
            snprintf(out, len, "%04d", ti->tm_year + 1900);
        } else {
            strncpy(out, "1992", len);
        }
    }

    // Legacy wrappers — return String (for gradual migration)
    String getCurrentTime() const { char b[8]; getTime(b, sizeof(b)); return String(b); }
    String getCurrentTimeFull() const { char b[10]; getTimeFull(b, sizeof(b)); return String(b); }
    String getCurrentDateDDMM() const { char b[8]; getDateDDMM(b, sizeof(b)); return String(b); }
    String getCurrentYear() const { char b[6]; getYear(b, sizeof(b)); return String(b); }

    void setTheme(int idx) {
        if (!ALL_THEMES.empty() && idx >= 0 && idx < ALL_THEMES.size()) {
            themeIndex = idx;
            theme = &ALL_THEMES[idx];
        } else {
            themeIndex = 0;
            theme = !ALL_THEMES.empty() ? &ALL_THEMES[0] : &THEME_FALLBACK;
        }
    }
};

// =============================================================
// BASE APP CLASS
// =============================================================
class App {
public:
    virtual ~App() {}
    virtual void setup(AppContext* ctx, String args = "") {}
    virtual void draw(AppContext* ctx) = 0;
    virtual void drawStatus(AppContext* ctx) = 0;
    virtual bool handleInput(AppContext* ctx, char key, bool isDel, bool isEnter) = 0;
};
