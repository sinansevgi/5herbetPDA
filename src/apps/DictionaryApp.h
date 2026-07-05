#pragma once
#include "../App.h"
#include <SD.h>
#include <vector>

struct DictIndexEntry {
    char word[28];
    uint32_t offset;
    uint32_t length;
};

class DictionaryApp : public App {
private:
    String searchQuery = "";
    String definitionText = "";
    std::vector<String> wrappedLines;
    
    // Status states:
    // 0 = Idle / Welcome
    // 1 = Searching
    // 2 = Word found
    // 3 = Word not found
    int stateStatus = 0;
    String activeWord = "";
    
    int scrollOffset = 0;
    const int maxCharsPerLine = 48; // (320 - 20) / 6 = 50, use 48 to leave margin for scrollbar
    const int maxVisibleLines = 14; // y=60 to y=210 is 150px, each char is 8px high with line spacing (10px total)
    const int lineH = 11;

    void performSearch(AppContext* ctx) {
        if (searchQuery.length() == 0) return;
        
        scrollOffset = 0;
        wrappedLines.clear();
        stateStatus = 1;
        
        // Trim query
        searchQuery.trim();
        activeWord = searchQuery;
        
        if (!ctx->sdAvailable) {
            stateStatus = 3;
            definitionText = "No MicroSD card detected. Please insert SD card with dict.bin database.";
            wrappedLines = wrapText(definitionText, maxCharsPerLine);
            return;
        }

        File f = SD.open("/5herbetPDA/dict.bin", FILE_READ);
        if (!f) {
            stateStatus = 3;
            definitionText = "Database not found. Please run 'generate_dict.py' on PC and copy 'dict.bin' to /5herbetPDA/dict.bin on the SD card.";
            wrappedLines = wrapText(definitionText, maxCharsPerLine);
            return;
        }

        uint32_t entryCount = 0;
        f.read((uint8_t*)&entryCount, 4);

        int low = 0;
        int high = entryCount - 1;
        bool found = false;

        // Convert query to lower case to match index
        char target[28];
        strncpy(target, searchQuery.c_str(), 27);
        target[27] = '\0';
        for (int i = 0; target[i]; i++) target[i] = tolower(target[i]);

        while (low <= high) {
            int mid = low + (high - low) / 2;
            f.seek(4 + mid * sizeof(DictIndexEntry));
            
            DictIndexEntry entry;
            f.read((uint8_t*)&entry, sizeof(DictIndexEntry));

            int cmp = strcmp(target, entry.word);
            if (cmp == 0) {
                // Word matched! Read definition
                f.seek(entry.offset);
                char* buf = new char[entry.length + 1];
                f.read((uint8_t*)buf, entry.length);
                buf[entry.length] = '\0';
                
                definitionText = String(buf);
                delete[] buf;
                
                wrappedLines = wrapText(definitionText, maxCharsPerLine);
                stateStatus = 2;
                found = true;
                break;
            } else if (cmp < 0) {
                high = mid - 1;
            } else {
                low = mid + 1;
            }
        }
        f.close();

        if (!found) {
            stateStatus = 3;
            definitionText = "Word '" + searchQuery + "' not found in dictionary.";
            wrappedLines = wrapText(definitionText, maxCharsPerLine);
        }
    }

    std::vector<String> wrapText(const String& text, int charsPerLine) {
        std::vector<String> lines;
        int len = text.length();
        int i = 0;
        
        while (i < len) {
            int lineLen = 0;
            int lastSpace = -1;
            
            while (i + lineLen < len && lineLen < charsPerLine) {
                char c = text.charAt(i + lineLen);
                if (c == '\n') {
                    lineLen++;
                    break;
                }
                if (c == ' ') {
                    lastSpace = lineLen;
                }
                lineLen++;
            }
            
            if (i + lineLen < len && text.charAt(i + lineLen - 1) != '\n' && lineLen >= charsPerLine) {
                if (lastSpace != -1) {
                    lineLen = lastSpace + 1; // Wrap at space
                }
            }
            
            String line = text.substring(i, i + lineLen);
            line.replace("\n", "");
            lines.push_back(line);
            i += lineLen;
        }
        return lines;
    }

public:
    void setup(AppContext* ctx, String args = "") override {
        searchQuery = "";
        definitionText = "";
        wrappedLines.clear();
        stateStatus = 0;
        scrollOffset = 0;
        
        if (args != "") {
            searchQuery = args;
            performSearch(ctx);
        }
    }

    void draw(AppContext* ctx) override {
        auto& s = *ctx->extSprite;
        const Theme& th = *ctx->theme;
        s.fillScreen(th.bg);

        // Header bar
        char timeStr[8];
        ctx->getTime(timeStr, sizeof(timeStr));
        UI::drawHeader(s, th, ctx->userName.c_str(), timeStr);

        // Search Input Bar
        s.setTextColor(th.textFaint);
        s.setTextSize(1);
        s.setCursor(10, 26);
        s.print("SEARCH WORD:");

        UI::drawRecessedPanel(s, 10, 36, 300, 20, th);
        s.setTextColor(th.accentBright);
        s.setCursor(16, 42);
        s.print(searchQuery);
        
        // Caret blinking
        if (stateStatus != 1 && (millis() / 500) % 2 == 0) {
            int caretX = 16 + searchQuery.length() * 6;
            s.fillRect(caretX, 40, 6, 12, th.accent);
        }

        UI::drawSeparator(s, 4, 62, 312, th);

        // Content Area
        int drawY = 70;
        int drawX = 15;

        if (stateStatus == 0) {
            // Welcome screen
            s.setTextColor(th.textPrimary);
            s.setTextSize(1);
            s.setCursor(drawX, drawY);
            s.print("Welcome to English-English Dictionary!");
            s.setCursor(drawX, drawY + 16);
            s.setTextColor(th.textDim);
            s.print("Type a word in the search box above and");
            s.setCursor(drawX, drawY + 28);
            s.print("press ENTER to search.");
            s.setCursor(drawX, drawY + 44);
            s.setTextColor(th.textFaint);
            s.print("The app queries from a packed 'dict.bin' file");
            s.setCursor(drawX, drawY + 56);
            s.print("on the SD card in O(log N) lookup time.");
        } 
        else if (stateStatus == 1) {
            // Searching
            s.setTextColor(th.accent);
            s.setTextSize(2);
            s.setCursor(drawX, drawY + 20);
            s.print("Searching...");
        } 
        else if (stateStatus == 2 || stateStatus == 3) {
            // Display lines
            s.setTextSize(1);
            int visibleLinesCount = min((int)wrappedLines.size() - scrollOffset, maxVisibleLines);
            
            for (int i = 0; i < visibleLinesCount; i++) {
                int lineIndex = scrollOffset + i;
                int ly = drawY + i * lineH;
                
                // Color first word if we have query match
                if (stateStatus == 2 && lineIndex == 0) {
                    s.setTextColor(th.accentBright);
                } else {
                    s.setTextColor(stateStatus == 3 ? th.danger : th.textPrimary);
                }
                
                s.setCursor(drawX, ly);
                s.print(wrappedLines[lineIndex]);
            }

            // Draw scrollbar if content exceeds area
            if ((int)wrappedLines.size() > maxVisibleLines) {
                int scrollbarX = 308;
                int scrollbarY = 70;
                int scrollbarH = maxVisibleLines * lineH - 4;
                int scrollbarW = 3;
                
                s.fillRect(scrollbarX, scrollbarY, scrollbarW, scrollbarH, th.bgRecessed);
                
                int thumbH = max(4, (maxVisibleLines * scrollbarH) / (int)wrappedLines.size());
                int thumbY = scrollbarY + (scrollOffset * (scrollbarH - thumbH)) / ((int)wrappedLines.size() - maxVisibleLines);
                s.fillRect(scrollbarX, thumbY, scrollbarW, thumbH, th.accent);
            }
        }

        // Footer hint
        UI::drawFooter(s, th, "Type: A-Z  [ENT] Search  [DEL] Backspace  [; / .] Scroll  [ESC] Exit");
    }

    void drawStatus(AppContext* ctx) override {
        auto& is = *ctx->intDisplay;
        const Theme& th = *ctx->theme;
        is.fillScreen(th.bg);

        // Header line
        is.drawLine(10, 30, 230, 30, th.border);

        is.setTextColor(th.accent);
        is.setTextSize(2);
        is.setCursor(15, 10);
        is.print("DICTIONARY");

        if (stateStatus == 0) {
            is.setTextColor(th.textFaint);
            is.setTextSize(1);
            is.setCursor(15, 50);
            is.print("STANDBY");
            is.setCursor(15, 70);
            is.print("Ready to lookup words");
        } 
        else if (stateStatus == 1) {
            is.setTextColor(th.accentBright);
            is.setTextSize(1);
            is.setCursor(15, 50);
            is.print("SEARCHING...");
        } 
        else if (stateStatus == 2) {
            is.setTextColor(th.indicator);
            is.setTextSize(1);
            is.setCursor(15, 45);
            is.print("WORD DEFINED");

            is.setTextColor(th.textPrimary);
            is.setTextSize(3);
            is.setCursor(15, 65);
            
            // Truncate display word to fit status screen width
            String disp = activeWord;
            if (disp.length() > 10) disp = disp.substring(0, 8) + "..";
            is.print(disp);
            
            is.setTextColor(th.textFaint);
            is.setTextSize(1);
            is.setCursor(15, 105);
            is.printf("%d lines of definition", wrappedLines.size());
        } 
        else if (stateStatus == 3) {
            is.setTextColor(th.danger);
            is.setTextSize(1);
            is.setCursor(15, 50);
            is.print("NOT FOUND");
            
            is.setTextColor(th.textPrimary);
            is.setTextSize(2);
            is.setCursor(15, 70);
            String disp = activeWord;
            if (disp.length() > 14) disp = disp.substring(0, 12) + "..";
            is.print(disp);
        }
    }

    bool handleInput(AppContext* ctx, char key, bool isDel, bool isEnter) override {
        if (isEnter) {
            if (searchQuery.length() > 0) {
                performSearch(ctx);
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CONFIRM);
            } else {
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::ERROR);
            }
            return true;
        }

        if (isDel) {
            if (searchQuery.length() > 0) {
                searchQuery.remove(searchQuery.length() - 1);
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
            } else {
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::ERROR);
            }
            return true;
        }

        // Handle scrolling with arrow keys (which map to ';' and '.')
        if (key == ';') { // Up Arrow
            if (scrollOffset > 0) {
                scrollOffset--;
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
            }
            return true;
        }
        if (key == '.') { // Down Arrow
            if (scrollOffset < (int)wrappedLines.size() - maxVisibleLines) {
                scrollOffset++;
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
            }
            return true;
        }

        // Typing inputs (letters, numbers, space, hyphens)
        if ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9') || key == ' ' || key == '-') {
            if (searchQuery.length() < 24) { // Limit length
                searchQuery += key;
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
            }
            return true;
        }

        if (key == 27 || key == '`') {
            // Esc exits to desktop
            return false; 
        }

        return true;
    }
};
