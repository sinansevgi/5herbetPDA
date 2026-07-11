#pragma once
#include "../App.h"
#include <SD.h>
#include <WiFi.h>

class SettingsApp : public App {
private:
    enum class SettingsTab { NETWORK, SYSTEM, THEME, NUM_TABS };
    int currentTabIdx = 0;
    int selectedIndex = 0;
    bool isEditing = false;
    String editBuffer = "";

    int getNumItemsForTab(int tab) {
        switch(tab) {
            case 0: return 3; // SSID, Pass, Save
            case 1: return 7; // Screen, Sleep, TZ, Vol, Brightness, User, Save
            case 2: return 2; // Theme, Save
        }
        return 0;
    }

    const char* getItemLabel(int tab, int idx) {
        if (tab == 0) {
            if (idx == 0) return "WiFi SSID";
            if (idx == 1) return "WiFi Password";
            if (idx == 2) return "Save & Apply";
        } else if (tab == 1) {
            if (idx == 0) return "Screen Timeout";
            if (idx == 1) return "Sleep Timeout";
            if (idx == 2) return "Timezone (UTC)";
            if (idx == 3) return "Volume %";
            if (idx == 4) return "Brightness %";
            if (idx == 5) return "User Name";
            if (idx == 6) return "Save & Apply";
        } else if (tab == 2) {
            if (idx == 0) return "UI Theme";
            if (idx == 1) return "Save & Apply";
        }
        return "";
    }

    String getItemValue(AppContext* ctx, int tab, int idx) {
        if (tab == 0) {
            if (idx == 0) return ctx->wifiSSID.length() > 0 ? ctx->wifiSSID : "---";
            if (idx == 1) return ctx->wifiPass.length() > 0 ? "********" : "---";
        } else if (tab == 1) {
            if (idx == 0) return String(ctx->screenTimeoutMins) + " min";
            if (idx == 1) return String(ctx->sleepTimeoutMins) + " min";
            if (idx == 2) return (ctx->timezoneOffsetHours >= 0 ? "+" : "") + String(ctx->timezoneOffsetHours) + " h";
            if (idx == 3) return String(SysAudio.getVolume() * 100 / 255) + " %";
            if (idx == 4) return String(ctx->screenBrightness * 100 / 255) + " %";
            if (idx == 5) return ctx->userName;
        } else if (tab == 2) {
            if (idx == 0) return ctx->theme->name;
        }
        return "";
    }

    bool isTextField(int tab, int idx) {
        if (tab == 0 && (idx == 0 || idx == 1)) return true;
        if (tab == 1 && idx == 5) return true;
        return false;
    }

    void saveSettings(AppContext* ctx) {
        if (!ctx->sdAvailable) {
            ctx->showNotification("No SD card!");
            return;
        }
        File f = SD.open("/settings.conf", FILE_WRITE);
        if (!f) {
            ctx->showNotification("Write failed!");
            return;
        }
        f.print("SSID="); f.println(ctx->wifiSSID);
        f.print("PASS="); f.println(ctx->wifiPass);
        f.print("SCREEN="); f.println(ctx->screenTimeoutMins);
        f.print("SLEEP="); f.println(ctx->sleepTimeoutMins);
        f.print("TZ="); f.println(ctx->timezoneOffsetHours);
        f.print("USERNAME="); f.println(ctx->userName);
        f.print("THEME="); f.println(ctx->themeIndex);
        f.print("VOL="); f.println(SysAudio.getVolume() * 100 / 255);
        f.print("BRIGHTNESS="); f.println(ctx->screenBrightness);
        f.close();
        ctx->showNotification("Settings saved");
    }

    void connectWiFi(AppContext* ctx) {
        if (ctx->wifiSSID.length() > 0) {
            WiFi.begin(ctx->wifiSSID.c_str(), ctx->wifiPass.c_str());
            ctx->showNotification("Connecting WiFi...");
        }
    }

public:
    void setup(AppContext* ctx, String args) override {
        currentTabIdx = 0;
        selectedIndex = -1;
        isEditing = false;
    }

    void draw(AppContext* ctx) override {
        auto& s = *ctx->extSprite;
        const Theme& th = *ctx->theme;
        s.fillScreen(th.bg);

        // Header
        { char tb[8]; ctx->getTime(tb, sizeof(tb)); UI::drawHeader(s, th, ctx->userName.c_str(), tb); }

        // Tabs
        const char* tabNames[] = { "NETWORK", "SYSTEM", "THEME" };
        int tabW = 320 / 3;
        for (int i = 0; i < 3; i++) {
            bool active = (i == currentTabIdx);
            bool tabFocused = (selectedIndex == -1 && active);
            s.fillRect(i * tabW, 21, tabW, 20, active ? th.bg : th.bgRaised);
            s.drawRect(i * tabW, 21, tabW, 20, th.border);
            
            if (tabFocused) {
                s.fillRect(i * tabW + 2, 23, tabW - 4, 2, th.accent);
            }

            s.setTextSize(1);
            s.setTextColor(active ? (tabFocused ? th.accentBright : th.accent) : th.textDim);
            int tw = strlen(tabNames[i]) * 6;
            s.setCursor(i * tabW + (tabW - tw) / 2, 28);
            s.print(tabNames[i]);
            if (active) {
                // remove bottom border for active tab
                s.drawLine(i * tabW + 1, 40, i * tabW + tabW - 2, 40, th.bg);
            }
        }

        const int ROW_H = 22;
        const int ROW_X = 8;
        const int ROW_W = 304;
        const int START_Y = 50;

        int numItems = getNumItemsForTab(currentTabIdx);
        for (int i = 0; i < numItems; i++) {
            int y = START_Y + i * ROW_H;
            bool sel = (i == selectedIndex);
            bool ed  = sel && isEditing;

            if (i == numItems - 1) {
                // Save button
                if (sel) {
                    s.fillRect(ROW_X, y + 4, ROW_W, ROW_H - 2, th.accent);
                    s.setTextColor(th.bg);
                } else {
                    s.drawRect(ROW_X, y + 4, ROW_W, ROW_H - 2, th.border);
                    s.setTextColor(th.accent);
                }
                s.setTextSize(1);
                int tx = ROW_X + (ROW_W - 16 * 6) / 2;
                s.setCursor(tx, y + 10);
                s.print("[ SAVE & APPLY ]");
                continue;
            }

            // Normal item row
            if (sel) {
                s.fillRect(ROW_X, y, ROW_W, ROW_H - 2, th.selectBg);
                s.drawRect(ROW_X, y, ROW_W, ROW_H - 2, ed ? th.accentBright : th.accent);
                s.fillRect(ROW_X, y + 3, 2, ROW_H - 8, th.accent);
            } else {
                s.drawRect(ROW_X, y, ROW_W, ROW_H - 2, th.border);
            }

            s.setTextSize(1);
            s.setTextColor(sel ? th.textPrimary : th.textDim);
            s.setCursor(ROW_X + 8, y + 6);
            s.print(getItemLabel(currentTabIdx, i));

            String val = getItemValue(ctx, currentTabIdx, i);
            int valX = ROW_X + ROW_W - val.length() * 6 - 8;
            s.setTextColor(ed ? th.accentBright : (sel ? th.accent : th.accentDim));
            s.setCursor(valX, y + 6);
            s.print(val.c_str());

            if (ed) {
                UI::drawLED(s, ROW_X + ROW_W - 6, y + 3, th.accent);
            }
        }

        // Theme preview card
        if (currentTabIdx == 2 && selectedIndex == 0) {
            UI::drawRaisedPanel(s, 160 - 60, 90, 120, 80, th);
            s.fillRect(170 - 60, 100, 100, 40, th.bg); // mini screen
            s.fillRect(170 - 60, 100, 100, 10, th.bgRaised); // mini header
            s.fillRect(175 - 60, 115, 40, 20, th.bgPanel);
            s.drawRect(220 - 60, 115, 45, 8, th.selectBg);
            s.drawRect(220 - 60, 127, 45, 8, th.accent);
            s.setTextColor(th.textPrimary);
            s.setCursor(172 - 60, 102); s.print("Preview");
            s.setTextColor(th.accentBright);
            int lw = strlen(th.name) * 6;
            s.setCursor(160 - lw/2, 150); s.print(th.name);
        }

        // Text input modal
        if (isEditing && isTextField(currentTabIdx, selectedIndex)) {
            UI::drawRaisedPanel(s, 20, 80, 280, 64, th);
            s.setTextColor(th.textPrimary);
            s.setTextSize(1);
            s.setCursor(26, 88);
            s.print("Enter "); s.print(getItemLabel(currentTabIdx, selectedIndex));
            
            s.fillRect(26, 102, 268, 32, th.bgRecessed);
            s.drawRect(26, 102, 268, 32, th.border);
            
            s.setTextSize(2);
            s.setTextColor(th.accentBright);
            String disp = editBuffer + "_";
            if (disp.length() > 21) disp = disp.substring(disp.length() - 21);
            s.setCursor(32, 110);
            s.print(disp.c_str());
            s.setTextSize(1);
        }

        // Footer
        if (isEditing && isTextField(currentTabIdx, selectedIndex)) {
            UI::drawFooter(s, th, "Type value   ENTER Confirm   ESC Cancel");
        } else {
            UI::drawFooter(s, th, "ARROWS Nav/Adjust/Tab   ENTER Edit");
        }
    }

    void drawStatus(AppContext* ctx) override {
        auto& is = *ctx->intDisplay;
        const Theme& th = *ctx->theme;

        if (isEditing && selectedIndex >= 0 && isTextField(currentTabIdx, selectedIndex)) {
            is.setTextColor(th.accentBright);
            is.setTextSize(2);
            is.setCursor(10, 10);
            is.print("EDIT MODE");

            auto keys = M5Cardputer.Keyboard.keysState();
            
            // SHIFT badge
            if (keys.shift) {
                is.fillRect(10, 40, 80, 30, th.accent);
                is.setTextColor(th.bg);
            } else {
                is.drawRect(10, 40, 80, 30, th.border);
                is.setTextColor(th.textDim);
            }
            is.setCursor(20, 48); is.print("SHIFT");

            // FN badge
            if (keys.fn) {
                is.fillRect(100, 40, 60, 30, th.accent);
                is.setTextColor(th.bg);
            } else {
                is.drawRect(100, 40, 60, 30, th.border);
                is.setTextColor(th.textDim);
            }
            is.setCursor(115, 48); is.print("FN");

        } else if (currentTabIdx == 1 && selectedIndex >= 0 && selectedIndex < 5) { // Dials
            is.setTextColor(th.textFaint);
            is.setTextSize(1);
            is.setCursor(10, 10);
            is.print(getItemLabel(currentTabIdx, selectedIndex));
            
            is.setTextSize(3);
            is.setTextColor(th.accentBright);
            is.setCursor(10, 30);
            is.print(getItemValue(ctx, currentTabIdx, selectedIndex).c_str());

            int w = 220;
            is.drawRect(10, 70, w, 20, th.border);
            int fill = 0;
            if (selectedIndex == 0) fill = map(min(30, ctx->screenTimeoutMins), 1, 30, 0, w);
            if (selectedIndex == 1) fill = map(min(60, ctx->sleepTimeoutMins), 1, 60, 0, w);
            if (selectedIndex == 2) fill = map(ctx->timezoneOffsetHours, -12, 14, 0, w);
            if (selectedIndex == 3) fill = map(SysAudio.getVolume(), 0, 255, 0, w);
            if (selectedIndex == 4) fill = map(ctx->screenBrightness, 0, 255, 0, w);
            is.fillRect(10, 70, fill, 20, th.accent);

        } else if (currentTabIdx == 2 && selectedIndex >= 0) { // Theme Palette
            is.setTextColor(th.textFaint);
            is.setTextSize(1);
            is.setCursor(10, 10);
            is.print("THEME PALETTE");
            
            int bSize = 35;
            is.fillRect(10, 30, bSize, bSize, th.bg); is.drawRect(10, 30, bSize, bSize, th.border);
            is.fillRect(55, 30, bSize, bSize, th.bgPanel); is.drawRect(55, 30, bSize, bSize, th.border);
            is.fillRect(100, 30, bSize, bSize, th.accent); is.drawRect(100, 30, bSize, bSize, th.border);
            is.fillRect(145, 30, bSize, bSize, th.accentBright); is.drawRect(145, 30, bSize, bSize, th.border);

        } else { // Ambient
            is.setTextColor(th.accent);
            is.setTextSize(2);
            is.setCursor(10, 20);
            is.print("> SETTINGS");
            is.setTextSize(1);
            is.setTextColor(th.textFaint);
            is.setCursor(10, 50);
            is.print("TAB: "); 
            if (currentTabIdx == 0) is.print("NETWORK");
            else if (currentTabIdx == 1) is.print("SYSTEM");
            else is.print("THEME");
        }
    }

    bool handleInput(AppContext* ctx, char key, bool isDel, bool isEnter) override {
        if (isEditing && isTextField(currentTabIdx, selectedIndex)) {
            if (isEnter) {
                isEditing = false;
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CONFIRM);
                if (currentTabIdx == 0 && selectedIndex == 0) ctx->wifiSSID = editBuffer;
                if (currentTabIdx == 0 && selectedIndex == 1) ctx->wifiPass = editBuffer;
                if (currentTabIdx == 1 && selectedIndex == 5) ctx->userName = editBuffer.length() > 0 ? editBuffer : "STRANGER";
                editBuffer = "";
            } else if (key == 27) { // ESC
                isEditing = false;
            } else if (isDel) {
                if (editBuffer.length() > 0) editBuffer.remove(editBuffer.length() - 1);
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
            } else if (key >= 32 && key <= 126) {
                editBuffer += key;
            }
            return false;
        }

        if (key == '\t') {
            currentTabIdx = (currentTabIdx + 1) % 3;
            selectedIndex = 0;
            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
            return false;
        }

        int numItems = getNumItemsForTab(currentTabIdx);

        if (key == 's' || key == '.') {
            if (selectedIndex == -1) selectedIndex = 0;
            else if (selectedIndex == numItems - 1) selectedIndex = -1;
            else selectedIndex++;
            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
        } else if (key == 'w' || key == ';') {
            if (selectedIndex == -1) selectedIndex = numItems - 1;
            else if (selectedIndex == 0) selectedIndex = -1;
            else selectedIndex--;
            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
        } else if (key == 'a' || key == ',' || key == 'd' || key == '/') {
            int dir = (key == 'a' || key == ',') ? -1 : 1;
            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
            
            if (selectedIndex == -1) {
                currentTabIdx = (currentTabIdx + dir + 3) % 3;
            } else {
                bool isAdjustable = false;
                if (currentTabIdx == 1 && selectedIndex < 5) isAdjustable = true;
                if (currentTabIdx == 2 && selectedIndex == 0) isAdjustable = true;

                if (isAdjustable) {
                    if (currentTabIdx == 1) {
                        if (selectedIndex == 0) ctx->screenTimeoutMins = max(1, ctx->screenTimeoutMins + dir);
                        else if (selectedIndex == 1) ctx->sleepTimeoutMins = max(1, ctx->sleepTimeoutMins + dir);
                        else if (selectedIndex == 2) ctx->timezoneOffsetHours = max(-12, min(14, ctx->timezoneOffsetHours + dir));
                        else if (selectedIndex == 3) {
                            int v = (SysAudio.getVolume() * 100 / 255) + dir * 5;
                            SysAudio.setVolume(max(0, min(255, v * 255 / 100)));
                        }
                        else if (selectedIndex == 4) {
                            ctx->screenBrightness = max(0, min(255, ctx->screenBrightness + dir * 10));
                            ctx->setExtBrightness(ctx->screenBrightness);
                            M5Cardputer.Display.setBrightness(ctx->screenBrightness);
                        }
                    } else if (currentTabIdx == 2) {
                        if (selectedIndex == 0) {
                            int t = ctx->themeIndex + dir;
                            if (t < 0) t = ALL_THEMES.size() - 1;
                            if (t >= ALL_THEMES.size()) t = 0;
                            ctx->setTheme(t);
                        }
                    }
                }
            }
        } else if (isEnter) {
            if (selectedIndex == -1) {
                selectedIndex = 0;
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
            } else {
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
                if (selectedIndex == numItems - 1) {
                    saveSettings(ctx);
                    connectWiFi(ctx);
                } else if (isTextField(currentTabIdx, selectedIndex)) {
                    isEditing = true;
                    if (currentTabIdx == 0 && selectedIndex == 0) editBuffer = ctx->wifiSSID;
                    if (currentTabIdx == 0 && selectedIndex == 1) editBuffer = ctx->wifiPass;
                    if (currentTabIdx == 1 && selectedIndex == 5) editBuffer = ctx->userName;
                }
            }
        }
        return false;
    }
};
