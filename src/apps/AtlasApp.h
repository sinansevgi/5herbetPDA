#pragma once
#include "../App.h"
#include <SD.h>
#include <vector>
#include <math.h>
#include "../data/countries.h"
#include "../data/world_map_vector.h"

class AtlasApp : public App {
    // Pointer+count approach: points to DEFAULT_COUNTRIES or sdCountries.data()
    const CountryData* activeCountries = DEFAULT_COUNTRIES;
    int countryCount = ATLAS_COUNTRY_COUNT;

    // Only populated when loading from SD — owns the dynamic data
    std::vector<CountryData> sdCountries;
    std::vector<String> sdStrings;  // owns the char data for SD-loaded countries

    int cursorX = 160;
    int cursorY = 120;
    int closestIndex = -1;

    void loadDatabase(AppContext* ctx) {
        if (!ctx->sdAvailable) return;
        File f = SD.open("/5herbetPDA/Atlas/countries.csv", FILE_READ);
        if (!f) return;
        while (f.available()) {
            String line = f.readStringUntil('\n');
            line.trim();
            if (line.length() == 0) continue;
            int c1 = line.indexOf(',');
            int c2 = line.indexOf(',', c1+1);
            int c3 = line.indexOf(',', c2+1);
            int c4 = line.indexOf(',', c3+1);
            int c5 = line.indexOf(',', c4+1);
            int c6 = line.indexOf(',', c5+1);
            if (c6 != -1) {
                // Store strings in sdStrings, point CountryData at them
                sdStrings.push_back(line.substring(0, c1));
                const char* name = sdStrings.back().c_str();
                sdStrings.push_back(line.substring(c1+1, c2));
                const char* capital = sdStrings.back().c_str();
                sdStrings.push_back(line.substring(c2+1, c3));
                const char* population = sdStrings.back().c_str();
                sdStrings.push_back(line.substring(c3+1, c4));
                const char* currency = sdStrings.back().c_str();

                CountryData cd;
                cd.name = name;
                cd.capital = capital;
                cd.population = population;
                cd.currency = currency;
                cd.timezoneOffset = line.substring(c4+1, c5).toInt();
                cd.x = line.substring(c5+1, c6).toInt();
                cd.y = line.substring(c6+1).toInt();
                sdCountries.push_back(cd);
            }
        }
        f.close();
    }

    void updateClosest() {
        closestIndex = -1;
        float minDist = 20.0f;
        for (int i = 0; i < countryCount; i++) {
            float dx = activeCountries[i].x - cursorX;
            float dy = activeCountries[i].y - cursorY;
            float d = sqrt(dx*dx + dy*dy);
            if (d < minDist) { minDist = d; closestIndex = i; }
        }
    }

public:
    ~AtlasApp() override {}

    void setup(AppContext* ctx, String args) override {
        bool loadedFromSD = false;
        if (ctx->sdAvailable) {
            File f = SD.open("/5herbetPDA/Atlas/countries.csv", FILE_READ);
            if (f) {
                f.close();
                loadDatabase(ctx);
                if (!sdCountries.empty()) {
                    activeCountries = sdCountries.data();
                    countryCount = sdCountries.size();
                    loadedFromSD = true;
                }
            }
        }
        if (!loadedFromSD) {
            // Use built-in flash data directly — zero heap allocation
            activeCountries = DEFAULT_COUNTRIES;
            countryCount = ATLAS_COUNTRY_COUNT;
            ctx->showNotification("Fallback map data");
        }
    }

    void draw(AppContext* ctx) override {
        auto& s = *ctx->extSprite;
        const Theme& th = *ctx->theme;

        // Vector map background
        s.fillScreen(th.bgRecessed);
        
        // Grid
        for (int i = 0; i < 320; i += 40) s.drawLine(i, 0, i, 240, th.border);
        for (int j = 0; j < 240; j += 40) s.drawLine(0, j, 320, j, th.border);
        
        // Draw vector world map outline
        int last_x = -1, last_y = -1;
        for (int i = 0; WORLD_MAP_VECTOR[i] != -2; i += 2) {
            int px = WORLD_MAP_VECTOR[i];
            int py = WORLD_MAP_VECTOR[i+1];
            if (px == -1) {
                last_x = -1;
            } else {
                if (last_x != -1) {
                    s.drawLine(last_x, last_y, px, py, th.accentDim);
                }
                last_x = px;
                last_y = py;
            }
        }

        // Country dots (brighter so they stand out over lines)
        for (int i = 0; i < countryCount; i++) {
            s.fillRect(activeCountries[i].x-1, activeCountries[i].y-1, 3, 3, th.accentBright);
        }

        // Crosshair cursor
        s.drawLine(cursorX-6, cursorY, cursorX-2, cursorY, th.accent);
        s.drawLine(cursorX+2, cursorY, cursorX+6, cursorY, th.accent);
        s.drawLine(cursorX, cursorY-6, cursorX, cursorY-2, th.accent);
        s.drawLine(cursorX, cursorY+2, cursorX, cursorY+6, th.accent);
        // Center dot
        s.drawPixel(cursorX, cursorY, th.accentBright);

        // Highlight selected dot
        if (closestIndex != -1) {
            s.drawCircle(activeCountries[closestIndex].x - 1, activeCountries[closestIndex].y - 1, 5, th.accentBright);
            s.drawCircle(activeCountries[closestIndex].x - 1, activeCountries[closestIndex].y - 1, 6, th.accent);
        }

        // Country info overlay
        if (closestIndex != -1) {
            const CountryData& c = activeCountries[closestIndex];

            int boxW = 160, boxH = 72;
            int boxX = cursorX + 10, boxY = cursorY + 10;
            if (boxX + boxW > 320) boxX = cursorX - 10 - boxW;
            if (boxY + boxH > 240) boxY = cursorY - 10 - boxH;

            // Overlay panel
            s.fillRect(boxX, boxY, boxW, boxH, th.bgRaised);
            s.drawRect(boxX, boxY, boxW, boxH, th.accent);
            // Top accent line
            s.drawLine(boxX+1, boxY+1, boxX+boxW-2, boxY+1, th.accent);

            // Country name
            s.setTextColor(th.accent);
            s.setTextSize(1);
            s.setCursor(boxX+4, boxY+5);
            s.print(c.name);

            // Details
            s.setTextColor(th.textDim);
            s.setCursor(boxX+4, boxY+18);
            s.print("Cap: ");
            s.setTextColor(th.textPrimary);
            s.print(c.capital);

            s.setTextColor(th.textDim);
            s.setCursor(boxX+4, boxY+30);
            s.print("Pop: ");
            s.setTextColor(th.textPrimary);
            s.print(c.population);

            s.setTextColor(th.textDim);
            s.setCursor(boxX+4, boxY+42);
            s.print("Cur: ");
            s.setTextColor(th.textPrimary);
            s.print(c.currency);

            // Local time
            unsigned long nowEpoch = ctx->epochBase + (millis() - ctx->epochMillis) / 1000UL;
            long locEpoch = (long)nowEpoch + (long)c.timezoneOffset * 3600L;
            int h = ((locEpoch / 3600) % 24 + 24) % 24;
            int m = ((locEpoch / 60) % 60 + 60) % 60;
            char tbuf[8];
            snprintf(tbuf, sizeof(tbuf), "%02d:%02d", h, m);
            s.setTextColor(th.accent);
            s.setCursor(boxX+4, boxY+56);
            s.print("Time: ");
            s.print(tbuf);
        }

        // Top-left coordinate display
        s.fillRect(0, 0, 60, 12, th.bgRaised);
        s.drawRect(0, 0, 60, 12, th.border);
        s.setTextColor(th.textFaint);
        s.setTextSize(1);
        s.setCursor(2, 2);
        s.printf("%d,%d", cursorX, cursorY);
    }

    void drawStatus(AppContext* ctx) override {
        auto& is = *ctx->intDisplay;
        const Theme& th = *ctx->theme;

        if (closestIndex != -1) {
            const CountryData& c = activeCountries[closestIndex];

            // Local time calculation
            unsigned long nowEpoch = ctx->epochBase + (millis() - ctx->epochMillis) / 1000UL;
            long locEpoch = (long)nowEpoch + (long)c.timezoneOffset * 3600L;
            int h = ((locEpoch / 3600) % 24 + 24) % 24;
            int m = ((locEpoch / 60) % 60 + 60) % 60;
            char tbuf[8];
            snprintf(tbuf, sizeof(tbuf), "%02d:%02d", h, m);

            is.setTextColor(th.accentBright);
            is.setTextSize(2);
            is.setCursor(10, 15);
            is.print(c.name);

            is.setTextColor(th.textDim);
            is.setTextSize(1);
            is.setCursor(10, 45);
            is.printf("UTC %+d", c.timezoneOffset);

            is.setTextColor(th.textPrimary);
            is.setTextSize(4);
            is.setCursor(10, 65);
            is.print(tbuf);

            is.setTextColor(th.textFaint);
            is.setTextSize(1);
            is.setCursor(10, 115);
            is.print("[ENTER] to pin");
        } else {
            is.setTextColor(th.accentDim);
            is.setTextSize(3);
            is.setCursor(10, 40);
            is.print("ATLAS");

            is.setTextColor(th.textFaint);
            is.setTextSize(1);
            is.setCursor(10, 80);
            is.print("WASD to explore");
        }
    }

    bool handleInput(AppContext* ctx, char key, bool isDel, bool isEnter) override {
        int speed = 3; // Reduced from 5 to allow finer selection in dense regions like Europe
        bool moved = false;
        if (key == 'w' || key == ';') { cursorY -= speed; moved = true; }
        if (key == 's' || key == '.') { cursorY += speed; moved = true; }
        if (key == 'a' || key == ',') { cursorX -= speed; moved = true; }
        if (key == 'd' || key == '/') { cursorX += speed; moved = true; }
        if (moved) SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
        
        if (cursorX < 0) cursorX = 0;
        if (cursorX > 320) cursorX = 320;
        if (cursorY < 0) cursorY = 0;
        if (cursorY > 240) cursorY = 240;
        
        int oldClosest = closestIndex;
        updateClosest();
        if (oldClosest != closestIndex && closestIndex != -1) {
            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::SELECT);
        }
        
        if (isEnter) {
            if (closestIndex != -1) {
                ctx->pinnedWorldClockName = activeCountries[closestIndex].name;
                ctx->pinnedWorldClockOffset = activeCountries[closestIndex].timezoneOffset;
                ctx->showNotification(ctx->pinnedWorldClockName + " pinned!");
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CONFIRM);
            } else {
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::ERROR);
            }
        }
        return false;
    }
};
