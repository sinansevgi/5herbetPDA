#pragma once
#include "../App.h"
#include <vector>

#include "WebUIServer.h"
#include "DictionaryApp.h"

class UtilitiesApp : public App {
private:
    enum class UtilMode {
        MENU,
        STOPWATCH,
        ALARMS,
        TIMER,
        POMODORO,
        WEBUI,
        DICTIONARY
    };

    UtilMode currentMode = UtilMode::MENU;
    int menuIndex = 0;
    static const int TOTAL_UTILS = 6;
    
    WebUIServerApp webServer;
    DictionaryApp dictApp;
    M5Canvas* intSprite = nullptr;
    
    // Stopwatch State
    bool swRunning = false;
    unsigned long swStartMillis = 0;
    unsigned long swElapsedMillis = 0;

    // Alarms State
    int alarmMenuIndex = 0;
    bool isEditingAlarm = false;
    int editField = 0; // 0=Hour, 1=Minute
    AlarmData tempAlarm;

    // Timer State
    bool tmRunning = false;
    bool tmRinging = false;
    unsigned long tmStartMillis = 0;
    unsigned long tmTargetMillis = 0; // The duration to count down
    unsigned long tmRemainingMillis = 0;
    bool isEditingTimer = false;
    int timerEditField = 0; // 0=H, 1=M, 2=S
    int tH = 0, tM = 0, tS = 0;

    // Pomodoro State
    enum class PomoState { STOPPED, FOCUS, SHORT_BREAK, LONG_BREAK };
    PomoState pomoState = PomoState::STOPPED;
    bool pomoRunning = false;
    unsigned long pomoStartMillis = 0;
    unsigned long pomoTargetMillis = 0;
    unsigned long pomoRemainingMillis = 0;
    int pomoCycleCount = 0;
    bool pomoRinging = false;

    const unsigned long POMO_FOCUS_MS = 25 * 60 * 1000;
    const unsigned long POMO_SHORT_BREAK_MS = 5 * 60 * 1000;
    const unsigned long POMO_LONG_BREAK_MS = 15 * 60 * 1000;

    // Helper functions

    void toggleStopwatch() {
        if (swRunning) {
            swElapsedMillis += (millis() - swStartMillis);
            swRunning = false;
        } else {
            swStartMillis = millis();
            swRunning = true;
        }
    }

    void resetStopwatch() {
        swRunning = false;
        swElapsedMillis = 0;
    }

    unsigned long getStopwatchElapsed() {
        if (swRunning) {
            return swElapsedMillis + (millis() - swStartMillis);
        }
        return swElapsedMillis;
    }

    void toggleTimer() {
        if (tmRunning) {
            // Pause
            tmRemainingMillis = tmTargetMillis - (millis() - tmStartMillis);
            tmRunning = false;
        } else {
            if (tmRemainingMillis == 0 && !tmRinging) {
                // First start
                tmRemainingMillis = (tH * 3600UL + tM * 60UL + tS) * 1000UL;
            }
            if (tmRemainingMillis > 0) {
                tmTargetMillis = tmRemainingMillis;
                tmStartMillis = millis();
                tmRunning = true;
                tmRinging = false;
            }
        }
    }

    void resetTimer() {
        tmRunning = false;
        tmRinging = false;
        tmRemainingMillis = 0;
    }

    void checkTimer(AppContext* ctx) {
        if (tmRunning) {
            unsigned long elapsed = millis() - tmStartMillis;
            if (elapsed >= tmTargetMillis) {
                tmRunning = false;
                tmRemainingMillis = 0;
                tmRinging = true;
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::ALERT);
            } else {
                tmRemainingMillis = tmTargetMillis - elapsed;
            }
        } else if (tmRinging) {
            // Periodic beep if still on the screen and ringing
            if ((millis() % 2000) < 50) {
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::ALERT);
            }
        }
    }

    void setPomoMode(PomoState state) {
        pomoState = state;
        pomoRunning = false;
        pomoRinging = false;
        if (state == PomoState::FOCUS) pomoRemainingMillis = POMO_FOCUS_MS;
        else if (state == PomoState::SHORT_BREAK) pomoRemainingMillis = POMO_SHORT_BREAK_MS;
        else if (state == PomoState::LONG_BREAK) pomoRemainingMillis = POMO_LONG_BREAK_MS;
        else pomoRemainingMillis = 0;
    }

    void togglePomodoro() {
        if (pomoState == PomoState::STOPPED) {
            setPomoMode(PomoState::FOCUS);
        }
        
        if (pomoRunning) {
            pomoRemainingMillis = pomoTargetMillis - (millis() - pomoStartMillis);
            pomoRunning = false;
        } else {
            if (pomoRemainingMillis > 0) {
                pomoTargetMillis = pomoRemainingMillis;
                pomoStartMillis = millis();
                pomoRunning = true;
                pomoRinging = false;
            }
        }
    }

    void resetPomodoro() {
        pomoState = PomoState::STOPPED;
        pomoRunning = false;
        pomoRinging = false;
        pomoRemainingMillis = 0;
        pomoCycleCount = 0;
    }

    void checkPomodoro(AppContext* ctx) {
        if (pomoRunning) {
            unsigned long elapsed = millis() - pomoStartMillis;
            if (elapsed >= pomoTargetMillis) {
                pomoRunning = false;
                pomoRemainingMillis = 0;
                pomoRinging = true;
                if (pomoState == PomoState::FOCUS) {
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::POMO_BREAK);
                } else {
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::POMO_FOCUS);
                }
            } else {
                pomoRemainingMillis = pomoTargetMillis - elapsed;
            }
        } else if (pomoRinging) {
            if ((millis() % 2000) < 50) {
                if (pomoState == PomoState::FOCUS) {
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::POMO_BREAK);
                } else {
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::POMO_FOCUS);
                }
            }
        }
    }

    void drawMenuCard(M5Canvas& s, int x, int y, int w, int h, const char* title, const char* desc, bool selected, const Theme& th) {
        if (selected) {
            s.fillRect(x, y, w, h, th.selectBg);
            s.drawRect(x, y, w, h, th.accent);
            s.fillRect(x, y, 4, h, th.accent); // Selection bar on left
            s.setTextColor(th.accentBright);
        } else {
            s.drawRect(x, y, w, h, th.border);
            s.setTextColor(th.textPrimary);
        }
        s.setTextSize(2);
        s.setCursor(x + 10, y + 8);
        s.print(title);
        
        s.setTextSize(1);
        if (selected) s.setTextColor(th.accent);
        else s.setTextColor(th.textFaint);
        s.setCursor(x + 10, y + 28);
        s.print(desc);
    }

public:
    ~UtilitiesApp() {
        if (intSprite) {
            intSprite->deleteSprite();
            delete intSprite;
        }
    }

    void setup(AppContext* ctx, String args) override {
        currentMode = UtilMode::MENU;
        if (!intSprite) {
            intSprite = new M5Canvas(ctx->intDisplay);
            intSprite->createSprite(240, 135);
        }
    }

    void draw(AppContext* ctx) override {
        auto& s = *ctx->extSprite;
        const Theme& th = *ctx->theme;

        // Background tasks
        checkTimer(ctx);
        checkPomodoro(ctx);

        s.fillScreen(th.bg);
        { char tb[8]; ctx->getTime(tb, sizeof(tb)); UI::drawHeader(s, th, ctx->userName.c_str(), tb); }

        if (currentMode == UtilMode::MENU) {
            const int cardW = 145;
            const int cardH = 50;
            const int startX = 10;
            const int startY = 30;
            const int padding = 10;

            const char* titles[6] = {"Stopwatch", "Alarms", "Timer", "Pomodoro", "Web Server", "Dictionary"};
            const char* descs[6]  = {"Track elapsed time", "Set wake alarms", "Countdown timer", "Focus & break", "Manage PDA files", "Offline word lookup"};

            for (int i = 0; i < TOTAL_UTILS; i++) {
                int col = i % 2;
                int row = i / 2;
                int x = startX + col * (cardW + padding);
                int y = startY + row * (cardH + padding);
                drawMenuCard(s, x, y, cardW, cardH, titles[i], descs[i], i == menuIndex, th);
            }
            UI::drawFooter(s, th, "Utilities Menu");
        } 
        else if (currentMode == UtilMode::STOPWATCH) {
            s.setTextColor(th.textFaint);
            s.setTextSize(1);
            s.setCursor(10, 30);
            s.print("STOPWATCH");
            s.drawLine(10, 42, 310, 42, th.border);

            unsigned long ms = getStopwatchElapsed();
            unsigned long totalSecs = ms / 1000;
            unsigned long h = totalSecs / 3600;
            unsigned long m = (totalSecs / 60) % 60;
            unsigned long sec = totalSecs % 60;
            unsigned long frac = (ms % 1000) / 10;

            char buf[32];
            if (h > 0) snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", h, m, sec);
            else snprintf(buf, sizeof(buf), "%02lu:%02lu.%02lu", m, sec, frac);

            s.setTextColor(swRunning ? th.accentBright : th.accent);
            s.setTextSize(5); 
            int len = strlen(buf) * 30; 
            s.setCursor((320 - len) / 2, 100);
            s.print(buf);
            
            UI::drawFooter(s, th, "[ENTER] Start/Stop  [DEL] Reset  [ESC] Back");
        }
        else if (currentMode == UtilMode::ALARMS) {
            s.setTextColor(th.textFaint);
            s.setTextSize(1);
            s.setCursor(10, 30);
            s.print("ALARMS");
            s.drawLine(10, 42, 310, 42, th.border);

            if (ctx->alarms.empty()) {
                s.setTextColor(th.textDim);
                s.setCursor(10, 60);
                s.print("No alarms configured.");
            } else {
                for (size_t i = 0; i < ctx->alarms.size(); i++) {
                    int y = 60 + i * 25;
                    auto& a = ctx->alarms[i];
                    
                    if (i == alarmMenuIndex && !isEditingAlarm) {
                        s.fillRect(5, y - 5, 310, 25, th.selectBg);
                    }
                    
                    s.setTextColor(a.enabled ? th.accent : th.textDim);
                    s.setTextSize(2);
                    s.setCursor(15, y);
                    s.printf("%02d:%02d", a.hour, a.minute);
                    
                    s.setTextSize(1);
                    s.setCursor(120, y + 4);
                    if (a.enabled) {
                        s.setTextColor(th.indicator);
                        s.print("[ ON ]");
                    } else {
                        s.setTextColor(th.textFaint);
                        s.print("[ OFF ]");
                    }
                }
            }

            if (isEditingAlarm) {
                s.fillRect(40, 60, 240, 120, th.bgRaised);
                s.drawRect(40, 60, 240, 120, th.border);
                
                s.setTextColor(th.textDim);
                s.setTextSize(1);
                s.setCursor(50, 70);
                s.print("ADD ALARM");
                
                s.setTextSize(5);
                s.setCursor(70, 100);
                
                if (editField == 0) {
                    s.setTextColor(th.accentBright); s.print("["); s.printf("%02d", tempAlarm.hour); s.print("]");
                } else {
                    s.setTextColor(th.textDim); s.print(" "); s.printf("%02d", tempAlarm.hour); s.print(" ");
                }
                
                s.setTextColor(th.textPrimary);
                s.print(":");
                
                if (editField == 1) {
                    s.setTextColor(th.accentBright); s.print("["); s.printf("%02d", tempAlarm.minute); s.print("]");
                } else {
                    s.setTextColor(th.textDim); s.print(" "); s.printf("%02d", tempAlarm.minute); s.print(" ");
                }
                UI::drawFooter(s, th, "[W/S] Edit [A/D] Sel [ENT] Save [ESC] Back");
            } else {
                UI::drawFooter(s, th, "[N] New [ENT] Tog [DEL] Del [ESC] Back");
            }
        }
        else if (currentMode == UtilMode::TIMER) {
            s.setTextColor(th.textFaint);
            s.setTextSize(1);
            s.setCursor(10, 30);
            s.print("TIMER");
            s.drawLine(10, 42, 310, 42, th.border);

            if (isEditingTimer) {
                s.fillRect(40, 60, 240, 120, th.bgRaised);
                s.drawRect(40, 60, 240, 120, th.border);
                s.setTextColor(th.textDim);
                s.setTextSize(1);
                s.setCursor(50, 70);
                s.print("SET TIMER");

                s.setTextSize(4);
                s.setCursor(30, 100);
                if (timerEditField == 0) { s.setTextColor(th.accentBright); s.printf("[%02d]", tH); }
                else { s.setTextColor(th.textDim); s.printf(" %02d ", tH); }
                
                s.setTextColor(th.textPrimary); s.print(":");
                
                if (timerEditField == 1) { s.setTextColor(th.accentBright); s.printf("[%02d]", tM); }
                else { s.setTextColor(th.textDim); s.printf(" %02d ", tM); }
                
                s.setTextColor(th.textPrimary); s.print(":");
                
                if (timerEditField == 2) { s.setTextColor(th.accentBright); s.printf("[%02d]", tS); }
                else { s.setTextColor(th.textDim); s.printf(" %02d ", tS); }
                
                UI::drawFooter(s, th, "[W/S] Edit [A/D] Sel [ENT] Save [ESC] Back");
            } else {
                unsigned long totalSecs = tmRemainingMillis / 1000;
                unsigned long h = totalSecs / 3600;
                unsigned long m = (totalSecs / 60) % 60;
                unsigned long sec = totalSecs % 60;

                char buf[32];
                snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", h, m, sec);

                if (tmRinging) s.setTextColor(th.danger);
                else s.setTextColor(tmRunning ? th.accentBright : th.accent);
                
                s.setTextSize(5); 
                int len = strlen(buf) * 30; 
                s.setCursor((320 - len) / 2, 100);
                s.print(buf);

                if (tmRinging) {
                    s.setTextSize(2);
                    s.setCursor(100, 150);
                    s.print("TIME'S UP!");
                }
                
                if (tmRinging) UI::drawFooter(s, th, "[ANY] Stop Alarm");
                else UI::drawFooter(s, th, "[S] Set [ENT] Start/Pause [DEL] Rst [ESC] Back");
            }
        }
        else if (currentMode == UtilMode::POMODORO) {
            s.setTextColor(th.textFaint);
            s.setTextSize(1);
            s.setCursor(10, 30);
            s.print("POMODORO TIMER");
            s.drawLine(10, 42, 310, 42, th.border);

            unsigned long totalSecs = pomoRemainingMillis / 1000;
            unsigned long m = (totalSecs / 60) % 60;
            unsigned long sec = totalSecs % 60;

            char buf[32];
            snprintf(buf, sizeof(buf), "%02lu:%02lu", m, sec);

            // Draw Mode Header
            s.setTextSize(2);
            if (pomoState == PomoState::FOCUS) {
                s.setTextColor(th.accentBright);
                s.setCursor(100, 60);
                s.print("FOCUS MODE");
            } else if (pomoState == PomoState::SHORT_BREAK || pomoState == PomoState::LONG_BREAK) {
                s.setTextColor(th.indicator);
                s.setCursor(100, 60);
                s.print("BREAK TIME");
            } else {
                s.setTextColor(th.textDim);
                s.setCursor(110, 60);
                s.print("READY");
            }

            // Draw Countdown
            if (pomoRinging) s.setTextColor(th.danger);
            else s.setTextColor(th.textPrimary);
            
            s.setTextSize(6); 
            int len = strlen(buf) * 36; 
            s.setCursor((320 - len) / 2, 100);
            s.print(buf);

            if (pomoRinging) {
                s.setTextSize(2);
                s.setCursor(85, 160);
                s.print("SESSION DONE!");
                UI::drawFooter(s, th, "[ANY] Stop Alarm");
            } else {
                UI::drawFooter(s, th, "[F] Foc [B] Brk [ENT] Tog [DEL] Rst [ESC] Back");
            }
        }
        else if (currentMode == UtilMode::WEBUI) {
            webServer.handleClient();
            
            s.setTextColor(th.textFaint);
            s.setTextSize(1);
            s.setCursor(10, 30);
            s.print("WEB SERVER");
            s.drawLine(10, 42, 310, 42, th.border);
            
            if (WiFi.status() == WL_CONNECTED) {
                if (!webServer.active()) webServer.start();
                s.setTextColor(th.indicator);
                s.setCursor(10, 60);
                s.print("Connected!");
                s.setTextColor(th.accentBright);
                s.setTextSize(2);
                s.setCursor(10, 80);
                s.print(WiFi.localIP().toString());
                s.setTextSize(1);
                s.setTextColor(th.textDim);
                s.setCursor(10, 110);
                s.print("Visit this IP in your browser");
            } else {
                if (webServer.active()) webServer.stop();
                if (ctx->wifiSSID.length() == 0) {
                    s.setTextColor(th.danger);
                    s.setCursor(10, 60);
                    s.print("WiFi not configured in Settings!");
                } else {
                    s.setTextColor(th.textDim);
                    s.setCursor(10, 60);
                    s.print("Connecting to WiFi...");
                }
            }
            UI::drawFooter(s, th, "[ESC] Stop & Back");
        }
        else if (currentMode == UtilMode::DICTIONARY) {
            dictApp.draw(ctx);
        }
    }

    void drawStatus(AppContext* ctx) override {
        if (!intSprite) return;
        auto& is = *intSprite;
        const Theme& th = *ctx->theme;

        is.fillScreen(th.bg);

        if (currentMode == UtilMode::MENU) {
            is.setTextColor(th.accentBright);
            is.setTextSize(3);
            is.setTextDatum(MC_DATUM);
            
            const char* titles[5] = {"STOPWATCH", "ALARMS", "TIMER", "POMODORO", "WEB UI"};
            is.drawString(titles[menuIndex], 120, 67);
            is.setTextDatum(TL_DATUM);
        } 
        else if (currentMode == UtilMode::STOPWATCH) {
            if (swRunning || getStopwatchElapsed() > 0) {
                unsigned long ms = getStopwatchElapsed();
                unsigned long totalSecs = ms / 1000;
                unsigned long h = totalSecs / 3600;
                unsigned long m = (totalSecs / 60) % 60;
                unsigned long sec = totalSecs % 60;
                unsigned long frac = (ms % 1000) / 10;

                char buf[32];
                if (h > 0) snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", h, m, sec);
                else snprintf(buf, sizeof(buf), "%02lu:%02lu.%02lu", m, sec, frac);

                is.setTextColor(swRunning ? th.accentBright : th.accent);
                is.setTextDatum(MC_DATUM);
                is.setTextSize(4);
                is.drawString(buf, 120, 67);
                is.setTextDatum(TL_DATUM);
            } else {
                is.setTextColor(th.textDim);
                is.setTextDatum(MC_DATUM);
                is.setTextSize(3);
                is.drawString("STOPWATCH", 120, 67);
                is.setTextDatum(TL_DATUM);
            }
        }
        else if (currentMode == UtilMode::ALARMS) {
            is.setTextColor(th.accent);
            is.setTextDatum(MC_DATUM);
            is.setTextSize(3);
            is.drawString("ALARMS", 120, 67);
            
            if (!ctx->alarms.empty()) {
                int onCount = 0;
                for (auto& a : ctx->alarms) {
                    if (a.enabled) onCount++;
                }
                is.setTextSize(2);
                is.setTextColor(th.textPrimary);
                char buf[32];
                snprintf(buf, sizeof(buf), "%d ACTIVE", onCount);
                is.drawString(buf, 120, 95);
            }
            is.setTextDatum(TL_DATUM);
        }
        else if (currentMode == UtilMode::TIMER) {
            unsigned long totalSecs = tmRemainingMillis / 1000;
            unsigned long h = totalSecs / 3600;
            unsigned long m = (totalSecs / 60) % 60;
            unsigned long sec = totalSecs % 60;

            char buf[32];
            if (h > 0) snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", h, m, sec);
            else snprintf(buf, sizeof(buf), "%02lu:%02lu", m, sec);

            is.setTextDatum(MC_DATUM);
            if (tmRinging) {
                is.setTextColor(th.danger);
                is.setTextSize(4);
                is.drawString("00:00", 120, 50);
                is.setTextSize(2);
                is.drawString("TIME'S UP", 120, 90);
            } else if (tmRunning || tmRemainingMillis > 0) {
                is.setTextColor(tmRunning ? th.accentBright : th.accent);
                is.setTextSize(5);
                is.drawString(buf, 120, 67);
            } else {
                is.setTextColor(th.textDim);
                is.setTextSize(4);
                is.drawString(buf, 120, 67);
            }
            is.setTextDatum(TL_DATUM);
        }
        else if (currentMode == UtilMode::POMODORO) {
            unsigned long totalSecs = pomoRemainingMillis / 1000;
            unsigned long m = (totalSecs / 60) % 60;
            unsigned long sec = totalSecs % 60;

            char buf[32];
            snprintf(buf, sizeof(buf), "%02lu:%02lu", m, sec);

            is.setTextDatum(MC_DATUM);
            if (pomoRinging) {
                is.setTextColor(th.danger);
                is.setTextSize(4);
                is.drawString(buf, 120, 50);
                is.setTextSize(2);
                is.drawString("DONE", 120, 90);
            } else {
                if (pomoState == PomoState::FOCUS) is.setTextColor(th.danger);
                else if (pomoState == PomoState::SHORT_BREAK || pomoState == PomoState::LONG_BREAK) is.setTextColor(th.indicator);
                else is.setTextColor(th.textDim);
                
                is.setTextSize(5);
                is.drawString(buf, 120, 67);
                
                if (pomoState != PomoState::STOPPED) {
                    float progress = 1.0f - ((float)pomoRemainingMillis / (float)pomoTargetMillis);
                    int angle = progress * 360;
                    is.drawArc(120, 67, 60, 55, 0, angle, th.accent);
                }

                is.setTextSize(2);
                if (pomoState == PomoState::FOCUS) is.drawString("FOCUS", 120, 20);
                else if (pomoState != PomoState::STOPPED) is.drawString("BREAK", 120, 20);
            }
            is.setTextDatum(TL_DATUM);
        }
        else if (currentMode == UtilMode::WEBUI) {
            is.setTextDatum(MC_DATUM);
            if (webServer.active()) {
                int r = 10 + (sin(millis() / 200.0) + 1.0) * 10;
                is.fillCircle(120, 50, r, th.indicator);
                is.setTextColor(th.indicator);
                is.setTextSize(2);
                is.drawString("SERVER ONLINE", 120, 90);
                is.setTextSize(1);
                is.drawString(WiFi.localIP().toString().c_str(), 120, 110);
            } else {
                is.setTextColor(th.textDim);
                is.setTextSize(2);
                is.drawString("WEB SERVER", 120, 67);
            }
            is.setTextDatum(TL_DATUM);
        }
        else if (currentMode == UtilMode::DICTIONARY) {
            dictApp.drawStatus(ctx);
        }
        
        is.pushSprite(0, 0);
    }

    bool handleInput(AppContext* ctx, char key, bool isDel, bool isEnter) override {
        if (currentMode == UtilMode::DICTIONARY) {
            bool handled = dictApp.handleInput(ctx, key, isDel, isEnter);
            if (!handled) {
                currentMode = UtilMode::MENU;
            }
            return true;
        }

        if (currentMode == UtilMode::MENU) {
            if (key == 'w' || key == ';') {
                if (menuIndex >= 2) menuIndex -= 2;
            } else if (key == 's' || key == '.') {
                if (menuIndex + 2 < TOTAL_UTILS) menuIndex += 2;
                else if (menuIndex + 1 == TOTAL_UTILS - 1) menuIndex++; // Edge case for odd number of items, move to last if below is empty
            } else if (key == 'a' || key == ',') {
                if (menuIndex % 2 != 0) menuIndex--;
            } else if (key == 'd' || key == '/') {
                if (menuIndex % 2 == 0 && menuIndex + 1 < TOTAL_UTILS) menuIndex++;
            } else if (isEnter) {
                if (menuIndex == 0) currentMode = UtilMode::STOPWATCH;
                else if (menuIndex == 1) currentMode = UtilMode::ALARMS;
                else if (menuIndex == 2) currentMode = UtilMode::TIMER;
                else if (menuIndex == 3) currentMode = UtilMode::POMODORO;
                else if (menuIndex == 4) {
                    currentMode = UtilMode::WEBUI;
                    if (WiFi.status() != WL_CONNECTED && ctx->wifiSSID.length() > 0) {
                        WiFi.begin(ctx->wifiSSID.c_str(), ctx->wifiPass.c_str());
                    }
                }
                else if (menuIndex == 5) {
                    currentMode = UtilMode::DICTIONARY;
                    dictApp.setup(ctx);
                }
            } else if (key == 27 || key == '`') {
                return false;
            }
        } 
        else if (currentMode == UtilMode::STOPWATCH) {
            if (isEnter) toggleStopwatch();
            else if (isDel) resetStopwatch();
            else if (key == 27 || key == '`') currentMode = UtilMode::MENU;
        }
        else if (currentMode == UtilMode::ALARMS) {
            if (isEditingAlarm) {
                if (key == 'w' || key == ';') {
                    if (editField == 0) tempAlarm.hour = (tempAlarm.hour + 1) % 24;
                    else tempAlarm.minute = (tempAlarm.minute + 1) % 60;
                } else if (key == 's' || key == '.') {
                    if (editField == 0) tempAlarm.hour = (tempAlarm.hour - 1 + 24) % 24;
                    else tempAlarm.minute = (tempAlarm.minute - 1 + 60) % 60;
                } else if (key == 'a' || key == ',') editField = 0;
                else if (key == 'd' || key == '/') editField = 1;
                else if (isEnter) {
                    ctx->alarms.push_back(tempAlarm);
                    isEditingAlarm = false;
                } else if (key == 27 || key == '`') isEditingAlarm = false;
            } else {
                if (key == 'w' || key == ';') {
                    if (!ctx->alarms.empty()) alarmMenuIndex = (alarmMenuIndex - 1 + ctx->alarms.size()) % ctx->alarms.size();
                } else if (key == 's' || key == '.') {
                    if (!ctx->alarms.empty()) alarmMenuIndex = (alarmMenuIndex + 1) % ctx->alarms.size();
                } else if (isEnter) {
                    if (!ctx->alarms.empty()) ctx->alarms[alarmMenuIndex].enabled = !ctx->alarms[alarmMenuIndex].enabled;
                } else if (key == 'n' || key == 'N') {
                    isEditingAlarm = true; editField = 0; tempAlarm = {12, 0, true, false};
                } else if (isDel) {
                    if (!ctx->alarms.empty()) {
                        ctx->alarms.erase(ctx->alarms.begin() + alarmMenuIndex);
                        if (alarmMenuIndex >= ctx->alarms.size() && alarmMenuIndex > 0) alarmMenuIndex--;
                    }
                } else if (key == 27 || key == '`') {
                    currentMode = UtilMode::MENU;
                }
            }
        }
        else if (currentMode == UtilMode::TIMER) {
            if (isEditingTimer) {
                if (key == 'w' || key == ';') {
                    if (timerEditField == 0) tH = (tH + 1) % 100;
                    else if (timerEditField == 1) tM = (tM + 1) % 60;
                    else tS = (tS + 1) % 60;
                } else if (key == 's' || key == '.') {
                    if (timerEditField == 0) tH = (tH - 1 + 100) % 100;
                    else if (timerEditField == 1) tM = (tM - 1 + 60) % 60;
                    else tS = (tS - 1 + 60) % 60;
                } else if (key == 'a' || key == ',') {
                    timerEditField = (timerEditField - 1 + 3) % 3;
                } else if (key == 'd' || key == '/') {
                    timerEditField = (timerEditField + 1) % 3;
                } else if (isEnter) {
                    tmRemainingMillis = (tH * 3600UL + tM * 60UL + tS) * 1000UL;
                    isEditingTimer = false;
                } else if (key == 27 || key == '`') {
                    isEditingTimer = false;
                }
            } else {
                if (tmRinging) {
                    // Any key stops the ringing
                    tmRinging = false;
                    tmRemainingMillis = 0;
                    return true;
                }
                
                if (isEnter) toggleTimer();
                else if (key == 's' || key == 'S') {
                    isEditingTimer = true;
                    timerEditField = 0;
                } else if (isDel) resetTimer();
                else if (key == 27 || key == '`') currentMode = UtilMode::MENU;
            }
        }
        else if (currentMode == UtilMode::POMODORO) {
            if (pomoRinging) {
                pomoRinging = false;
                if (pomoState == PomoState::FOCUS) pomoCycleCount++;
                pomoState = PomoState::STOPPED;
                pomoRemainingMillis = 0;
                return true;
            }
            
            if (isEnter) togglePomodoro();
            else if (key == 'f' || key == 'F') setPomoMode(PomoState::FOCUS);
            else if (key == 'b' || key == 'B') setPomoMode(PomoState::SHORT_BREAK);
            else if (isDel) resetPomodoro();
            else if (key == 27 || key == '`') currentMode = UtilMode::MENU;
        }
        else if (currentMode == UtilMode::WEBUI) {
            if (key == 27 || key == '`') {
                if (webServer.active()) webServer.stop();
                currentMode = UtilMode::MENU;
            }
        }
        return true;
    }
};
