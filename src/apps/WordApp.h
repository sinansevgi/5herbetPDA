#pragma once
#include "../App.h"
#include <SD.h>
#include <vector>

enum class WordState {
    FILE_BROWSER,
    NAMING_FILE,
    EDITING,
    TOOLBAR
};

class WordApp : public App {
private:
    WordState state = WordState::FILE_BROWSER;

    String noteText = "";
    int noteCursor = 0;
    bool noteSaved = true;
    String currentFile = "";
    
    // Text Selection
    int selectionStart = -1;
    int selectionEnd = -1;

    struct LineInfo {
        int startIndex;
        int length;
    };
    std::vector<LineInfo> displayLines;
    int desiredCol = 0;

    // File Browser
    struct FileEntry {
        String name;
        uint32_t size = 0;
        String dateStr = "";
    };
    std::vector<FileEntry> fileList;
    int fileSelectedIndex = 0;
    int fileScrollOffset = 0;
    int lastActionTime = 0;
    unsigned long sessionStartTime = 0;
    const String notesDir = "/5herbetPDA/Notes";

    // Toolbar (3x3 grid = 9 items)
    static const int TOOLBAR_ITEMS = 9;
    int toolbarMenuLevel = 0; // 0=Main, 1=Heading, 2=Align, 3=Color
    const char* toolbarLabels[TOOLBAR_ITEMS]       = {"Save", "Bold", "Undrln", "Color", "Head", "Align", "Close", "", ""};
    const char* toolbarLabelsHeading[TOOLBAR_ITEMS] = {"H1",   "H2",   "H3",     "",      "",     "",      "Back",  "", ""};
    const char* toolbarLabelsAlign[TOOLBAR_ITEMS]   = {"Left", "Center","Right", "",      "",     "",      "Back",  "", ""};
    const char* toolbarLabelsColor[TOOLBAR_ITEMS]   = {"Black","Red",   "Green", "Blue",  "",     "",      "Back",  "", ""};
    int toolbarIndex = 0;

    // Scrolling
    int scrollY = 0;

    void updateLayout() {
        displayLines.clear();
        int maxCharsPerLine = 46; // 312(width) - 40(margins) = 272 / 6 = 45.3
        int len = noteText.length();
        
        if (len == 0) {
            displayLines.push_back({0, 0});
            return;
        }

        int i = 0;
        int currentLineStart = 0;
        
        while (i < len) {
            int visibleChars = 0;
            int lastSpace = -1;
            
            // Skip line markers at start of line
            int headingLevel = 0;
            for(int j=i; j<len; j++) {
                if (noteText.charAt(j) == '#') headingLevel++;
                else if (noteText.charAt(j) == ' ' && headingLevel > 0) break;
                else { headingLevel = 0; break; }
            }
            if (headingLevel > 0) i += headingLevel + 1;
            else if (i + 1 < len && noteText.charAt(i) == '~' && noteText.charAt(i+1) == ' ') i += 2;
            else if (i + 1 < len && noteText.charAt(i) == '>' && noteText.charAt(i+1) == ' ') i += 2;

            while (i < len && visibleChars < maxCharsPerLine) {
                if (noteText.charAt(i) == '\n') {
                    i++; 
                    break;
                }
                
                // skip inline markers
                if (i + 1 < len && ((noteText.charAt(i) == '*' && noteText.charAt(i+1) == '*') || 
                                    (noteText.charAt(i) == '_' && noteText.charAt(i+1) == '_'))) {
                    i += 2; 
                    continue;
                }
                
                // skip color markers ~r~, ~g~, ~b~, ~k~
                if (i + 2 < len && noteText.charAt(i) == '~' && 
                    (noteText.charAt(i+1) == 'r' || noteText.charAt(i+1) == 'g' || noteText.charAt(i+1) == 'b' || noteText.charAt(i+1) == 'k') && 
                    noteText.charAt(i+2) == '~') {
                    i += 3;
                    continue;
                }

                if (noteText.charAt(i) == ' ') lastSpace = i;
                visibleChars++;
                i++;
            }
            
            if (i < len && noteText.charAt(i - 1) != '\n' && visibleChars >= maxCharsPerLine) {
                if (lastSpace != -1 && lastSpace >= currentLineStart) {
                    i = lastSpace + 1; // wrap after space
                }
            }
            
            displayLines.push_back({currentLineStart, i - currentLineStart});
            currentLineStart = i;
        }
        
        if (len > 0 && noteText.charAt(len-1) == '\n') {
            displayLines.push_back({len, 0});
        }
    }

    void getCursorPos(int& line, int& col) {
        line = 0;
        col = 0;
        if (displayLines.empty()) return;

        for (int i = 0; i < (int)displayLines.size(); i++) {
            int start = displayLines[i].startIndex;
            int len = displayLines[i].length;
            
            if (noteCursor >= start && noteCursor <= start + len) {
                if (noteCursor == start + len && i + 1 < (int)displayLines.size()) {
                    if (len > 0 && noteText.charAt(start + len - 1) == '\n') {
                        continue;
                    }
                    continue; 
                }
                line = i;
                col = noteCursor - start;
                return;
            }
        }
    }

    void updateDesiredCol() {
        int r, c;
        getCursorPos(r, c);
        desiredCol = c;
    }

    int getWordCount() {
        int count = 0;
        bool inWord = false;
        for (int i = 0; i < (int)noteText.length(); i++) {
            char c = noteText.charAt(i);
            if (isalnum(c)) {
                if (!inWord) { count++; inWord = true; }
            } else {
                inWord = false;
            }
        }
        return count;
    }

    bool isCursorBold() {
        bool bold = false;
        int lineStart = 0;
        for (int i=noteCursor-1; i>=0; i--) {
            if (noteText.charAt(i) == '\n') { lineStart = i + 1; break; }
        }
        for (int i = lineStart; i < noteCursor; i++) {
            if (i + 1 < noteText.length() && noteText.charAt(i) == '*' && noteText.charAt(i+1) == '*') {
                bold = !bold;
                i++;
            }
        }
        return bold;
    }
    
    bool isCursorUnderline() {
        bool ul = false;
        int lineStart = 0;
        for (int i=noteCursor-1; i>=0; i--) {
            if (noteText.charAt(i) == '\n') { lineStart = i + 1; break; }
        }
        for (int i = lineStart; i < noteCursor; i++) {
            if (i + 1 < noteText.length() && noteText.charAt(i) == '_' && noteText.charAt(i+1) == '_') {
                ul = !ul;
                i++;
            }
        }
        return ul;
    }

    int getCursorHeadingLevel() {
        int lineStart = 0;
        for (int i=noteCursor-1; i>=0; i--) {
            if (noteText.charAt(i) == '\n') { lineStart = i + 1; break; }
        }
        int level = 0;
        for (int i=lineStart; i < noteText.length(); i++) {
            if (noteText.charAt(i) == '#') level++;
            else if (noteText.charAt(i) == ' ' && level > 0) return level;
            else break;
        }
        return 0;
    }

    int getCursorAlign() { // 0=Left, 1=Center, 2=Right
        int lineStart = 0;
        for (int i=noteCursor-1; i>=0; i--) {
            if (noteText.charAt(i) == '\n') { lineStart = i + 1; break; }
        }
        if (lineStart + 1 < noteText.length()) {
            if (noteText.charAt(lineStart) == '~' && noteText.charAt(lineStart+1) == ' ') return 1;
            if (noteText.charAt(lineStart) == '>' && noteText.charAt(lineStart+1) == ' ') return 2;
        }
        return 0;
    }

    int getToolbarCount() {
        if (toolbarMenuLevel == 0) return 7;
        if (toolbarMenuLevel == 1) return 4;
        if (toolbarMenuLevel == 2) return 4;
        if (toolbarMenuLevel == 3) return 5;
        return 0;
    }

    const char* getToolbarLabel(int idx) {
        if (toolbarMenuLevel == 0) {
            static const char* labels[] = {"Save", "Bold", "Undrln", "Color", "Head", "Align", "Close"};
            return labels[idx];
        } else if (toolbarMenuLevel == 1) {
            static const char* labels[] = {"H1", "H2", "H3", "Back"};
            return labels[idx];
        } else if (toolbarMenuLevel == 2) {
            static const char* labels[] = {"Left", "Center", "Right", "Back"};
            return labels[idx];
        } else if (toolbarMenuLevel == 3) {
            static const char* labels[] = {"Black", "Red", "Green", "Blue", "Back"};
            return labels[idx];
        }
        return "";
    }

    String formatSize(uint32_t bytes) {
        if (bytes < 1024) return String(bytes) + " B";
        return String(bytes / 1024) + " KB";
    }

    void loadFileList(AppContext* ctx) {
        fileList.clear();
        fileList.push_back({"[NEW DOCUMENT]", 0, ""});
        if (ctx->sdAvailable) {
            if (!SD.exists(notesDir)) {
                SD.mkdir(notesDir);
            }
            File dir = SD.open(notesDir);
            if (dir) {
                while (File file = dir.openNextFile()) {
                    if (!file.isDirectory()) {
                        String name = file.name();
                        if (name.endsWith(".md") || name.endsWith(".txt")) {
                            uint32_t size = file.size();
                            time_t writeTime = file.getLastWrite();
                            struct tm* tmInfo = localtime(&writeTime);
                            char dateBuf[16];
                            if (tmInfo && tmInfo->tm_year > 70) {
                                snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d", tmInfo->tm_mday, tmInfo->tm_mon + 1);
                            } else {
                                strcpy(dateBuf, "--/--");
                            }
                            fileList.push_back({name, size, String(dateBuf)});
                        }
                    }
                }
            }
        }
        fileSelectedIndex = 0;
        fileScrollOffset = 0;
    }

    void loadNote(AppContext* ctx, String filename) {
        if (filename.startsWith(notesDir)) {
            currentFile = filename;
        } else if (filename.startsWith("/")) {
            currentFile = filename;
        } else {
            currentFile = notesDir + "/" + filename;
        }
        if (ctx->sdAvailable && SD.exists(currentFile)) {
            File f = SD.open(currentFile, FILE_READ);
            if (f) {
                noteText = f.readString();
                f.close();
                noteCursor = noteText.length();
                noteSaved = true;
            }
        } else {
            noteText = "";
            noteCursor = 0;
            noteSaved = false;
        }
        selectionStart = -1;
        selectionEnd = -1;
        updateLayout();
        updateDesiredCol();
        ctx->hasUnsavedWork = !noteSaved;
        sessionStartTime = millis();
    }

    void saveNote(AppContext* ctx) {
        if (currentFile == "" || currentFile == notesDir + "/[NEW DOCUMENT]") {
            currentFile = notesDir + "/Doc_" + ctx->getCurrentDateDDMM() + "_" + String(millis() % 10000) + ".md";
        }
        if (ctx->sdAvailable) {
            if (!SD.exists(notesDir)) {
                SD.mkdir(notesDir);
            }
            File f = SD.open(currentFile, FILE_WRITE);
            if (f) {
                f.print(noteText);
                f.close();
                noteSaved = true;
                ctx->hasUnsavedWork = false;
                ctx->showNotification("Saved!");
            } else {
                ctx->showNotification("Write error!");
            }
        } else {
            ctx->showNotification("No SD card");
        }
    }

    void insertAtCursor(AppContext* ctx, String str) {
        if (selectionStart != -1 && selectionEnd != -1 && selectionStart != selectionEnd) {
            int selMin = min(selectionStart, selectionEnd);
            int selMax = max(selectionStart, selectionEnd);
            noteText = noteText.substring(0, selMin) + str + noteText.substring(selMax);
            noteCursor = selMin + str.length();
            selectionStart = -1;
            selectionEnd = -1;
        } else {
            if (noteCursor >= (int)noteText.length()) {
                noteText += str;
            } else {
                noteText = noteText.substring(0, noteCursor) + str + noteText.substring(noteCursor);
            }
            noteCursor += str.length();
        }
        noteSaved = false;
        if (ctx) ctx->hasUnsavedWork = true;
        updateLayout();
        updateDesiredCol();
    }
    
    void clearSelection() {
        selectionStart = -1;
        selectionEnd = -1;
    }

public:
    void setup(AppContext* ctx, String args) override {
        state = WordState::FILE_BROWSER;
        loadFileList(ctx);
        sessionStartTime = millis();
        if (args != "") {
            loadNote(ctx, args);
            state = WordState::EDITING;
        }
    }

    void draw(AppContext* ctx) override {
        auto& s = *ctx->extSprite;
        const Theme& th = *ctx->theme;
        s.fillScreen(th.bg);

        { char tb[8]; ctx->getTime(tb, sizeof(tb)); UI::drawHeader(s, th, ctx->userName.c_str(), tb); }

        if (state == WordState::FILE_BROWSER) {
            s.setTextColor(th.textFaint, th.bg);
            s.setTextSize(1);
            s.setCursor(8, 26);
            s.print("DOCUMENTS - /Notes");

            UI::drawSeparator(s, 4, 36, 312, th);

            for (int i = fileScrollOffset; i < (int)fileList.size(); i++) {
                int visibleIndex = i - fileScrollOffset;
                if (visibleIndex >= 6) break; // max 6 items on screen (3 rows of 2 columns)

                int row = visibleIndex / 2;
                int col = visibleIndex % 2;
                int cellX = 8 + col * 158;
                int cellY = 44 + row * 56;
                int cellW = 146;
                int cellH = 50;

                bool isSelected = (i == fileSelectedIndex);

                if (isSelected) {
                    s.fillRect(cellX, cellY, cellW, cellH, th.bgRaised);
                    s.drawRect(cellX, cellY, cellW, cellH, th.accent);
                    s.drawRect(cellX + 1, cellY + 1, cellW - 2, cellH - 2, th.accentDim);
                } else {
                    s.fillRect(cellX, cellY, cellW, cellH, th.bgPanel);
                    s.drawRect(cellX, cellY, cellW, cellH, th.border);
                }

                if (i == 0) {
                    // NEW DOCUMENT button
                    s.setTextColor(isSelected ? th.accentBright : th.textPrimary);
                    s.setTextSize(1);
                    int plusX = cellX + 12;
                    int plusY = cellY + 17;
                    s.drawLine(plusX + 6, plusY + 2, plusX + 6, plusY + 14, isSelected ? th.accent : th.textDim);
                    s.drawLine(plusX + 2, plusY + 8, plusX + 10, plusY + 8, isSelected ? th.accent : th.textDim);

                    s.setCursor(cellX + 32, cellY + 21);
                    s.print("New Document");
                } else {
                    // Normal file item
                    // Draw document icon
                    int iconX = cellX + 10;
                    int iconY = cellY + 17;
                    uint16_t iconColor = isSelected ? th.accent : th.textDim;

                    s.drawRect(iconX, iconY, 12, 16, iconColor);
                    s.drawLine(iconX + 8, iconY, iconX + 12, iconY + 4, iconColor);
                    s.drawLine(iconX + 8, iconY, iconX + 8, iconY + 4, iconColor);
                    s.drawLine(iconX + 8, iconY + 4, iconX + 12, iconY + 4, iconColor);
                    s.drawLine(iconX + 2, iconY + 6, iconX + 10, iconY + 6, iconColor);
                    s.drawLine(iconX + 2, iconY + 9, iconX + 10, iconY + 9, iconColor);
                    s.drawLine(iconX + 2, iconY + 12, iconX + 7, iconY + 12, iconColor);

                    // Name
                    String displayName = fileList[i].name;
                    if (displayName.endsWith(".md")) displayName = displayName.substring(0, displayName.length() - 3);
                    else if (displayName.endsWith(".txt")) displayName = displayName.substring(0, displayName.length() - 4);
                    if (displayName.length() > 16) displayName = displayName.substring(0, 13) + "...";

                    s.setTextColor(isSelected ? th.accentBright : th.textPrimary);
                    s.setCursor(cellX + 28, cellY + 12);
                    s.print(displayName.c_str());

                    // Metadata size and date
                    String metaStr = formatSize(fileList[i].size) + "  |  " + fileList[i].dateStr;
                    s.setTextColor(th.textFaint);
                    s.setCursor(cellX + 28, cellY + 28);
                    s.print(metaStr.c_str());
                }
            }

            UI::drawFooter(s, th, "Arrows Navigate   ENTER Open   DEL Delete   ESC Exit");
        } else if (state == WordState::NAMING_FILE) {
            s.setTextColor(th.textFaint, th.bg);
            s.setTextSize(1);
            s.setCursor(8, 26);
            s.print("NEW DOCUMENT");

            UI::drawSeparator(s, 4, 36, 312, th);

            s.setTextColor(th.textPrimary, th.bg);
            s.setCursor(8, 50);
            s.print("Document Title:");
            
            UI::drawRecessedPanel(s, 8, 66, 304, 20, th);
            s.setCursor(14, 72);
            s.setTextColor(th.accentBright, th.bgRecessed);
            s.print(currentFile.c_str());
            
            if ((millis() / 500) % 2 == 0) {
                s.fillRect(14 + currentFile.length() * 6, 70, 6, 12, th.accent);
            }

            UI::drawFooter(s, th, "ENTER Confirm   ESC Cancel");
        } else {
            // EDITING or TOOLBAR
            s.setTextColor(th.textFaint, th.bg);
            s.setTextSize(1);
            s.setCursor(8, 26);
            s.print("WORD PROCESSOR");

            String fname = currentFile;
            if (fname.startsWith(notesDir + "/")) fname = fname.substring(notesDir.length() + 1);
            s.setTextColor(th.textDim, th.bg);
            s.setCursor(105, 26);
            s.print(fname.c_str());

            s.setCursor(280, 26);
            s.setTextColor(th.textFaint, th.bg);
            s.print(noteSaved ? "SAVED" : "EDIT");
            UI::drawLED(s, 310, 26, noteSaved ? th.indicator : th.accent);

            UI::drawSeparator(s, 4, 36, 312, th);
            
            // "Pocket Word" White Paper Theme using explicit 24-bit hex colors!
            uint32_t paperWhite = 0xFFFFFFU;
            s.fillRect(8, 40, 304, 180, paperWhite); 
            s.drawRect(8, 40, 304, 180, th.border);

            int lineH = 14; 
            int cLine, cCol;
            getCursorPos(cLine, cCol);
            
            int maxVisibleLines = 12; 
            if (cLine < scrollY) scrollY = cLine;
            if (cLine >= scrollY + maxVisibleLines) scrollY = cLine - maxVisibleLines + 1;
            
            int drawStartY = 46 - scrollY * lineH;
            
            s.setTextSize(1);
            
            bool isBold = false;
            bool isUnderline = false;
            
            // Pre-calculate color before visible lines
            uint32_t activeColor = 0x000000U;
            if (scrollY < (int)displayLines.size()) {
                int preLen = displayLines[scrollY].startIndex;
                for (int i = 0; i < preLen; i++) {
                    if (i + 2 < noteText.length() && noteText.charAt(i) == '~' && noteText.charAt(i+2) == '~') {
                        char cc = noteText.charAt(i+1);
                        if (cc == 'r') activeColor = 0xFF0000U;
                        else if (cc == 'g') activeColor = 0x008800U;
                        else if (cc == 'b') activeColor = 0x0000FFU;
                        else if (cc == 'k') activeColor = 0x000000U;
                    }
                }
            }

            for (int l = 0; l < (int)displayLines.size(); l++) {
                int ty = drawStartY + l * lineH;
                if (ty < 46 || ty >= 210) {
                    // Update active color even if hidden
                    int start = displayLines[l].startIndex;
                    int len = displayLines[l].length;
                    for (int i = start; i < start + len; i++) {
                        if (i + 2 < noteText.length() && noteText.charAt(i) == '~' && noteText.charAt(i+2) == '~') {
                            char cc = noteText.charAt(i+1);
                            if (cc == 'r') activeColor = 0xFF0000U;
                            else if (cc == 'g') activeColor = 0x008800U;
                            else if (cc == 'b') activeColor = 0x0000FFU;
                            else if (cc == 'k') activeColor = 0x000000U;
                        }
                    }
                    continue; 
                }

                int tx = 20; 
                int start = displayLines[l].startIndex;
                int len = displayLines[l].length;

                int hLevel = 0;
                int aLevel = 0; // 0=Left, 1=Center, 2=Right
                
                int textIdx = start;
                int checkHeading = 0;
                for(int j=start; j<start+len; j++) {
                    if (noteText.charAt(j) == '#') checkHeading++;
                    else if (noteText.charAt(j) == ' ' && checkHeading > 0) break;
                    else { checkHeading = 0; break; }
                }
                if (checkHeading > 0) {
                    hLevel = checkHeading;
                    textIdx += hLevel + 1;
                } else if (textIdx + 1 < start + len && noteText.charAt(textIdx) == '~' && noteText.charAt(textIdx+1) == ' ') {
                    aLevel = 1;
                    textIdx += 2;
                } else if (textIdx + 1 < start + len && noteText.charAt(textIdx) == '>' && noteText.charAt(textIdx+1) == ' ') {
                    aLevel = 2;
                    textIdx += 2;
                }

                if (aLevel > 0) {
                    int visibleLen = 0;
                    for (int i = textIdx; i < start + len; i++) {
                        if (i + 1 < start + len && ((noteText.charAt(i) == '*' && noteText.charAt(i+1) == '*') ||
                                                    (noteText.charAt(i) == '_' && noteText.charAt(i+1) == '_'))) {
                            i++; continue;
                        }
                        if (i + 2 < start + len && noteText.charAt(i) == '~' && noteText.charAt(i+2) == '~' &&
                            (noteText.charAt(i+1) == 'r' || noteText.charAt(i+1) == 'g' || noteText.charAt(i+1) == 'b' || noteText.charAt(i+1) == 'k')) {
                            i+=2; continue;
                        }
                        if (noteText.charAt(i) != '\n') visibleLen++;
                    }
                    if (aLevel == 1) { // Center
                        tx = 20 + ((280 - (visibleLen * 6)) / 2);
                    } else if (aLevel == 2) { // Right
                        tx = 300 - (visibleLen * 6);
                    }
                }

                for (int i = start; i < start + len; i++) {
                    // Check cursor
                    if (i == noteCursor) {
                        if (state == WordState::TOOLBAR || (millis() / 500) % 2 == 0) {
                            uint32_t caretColor = (state == WordState::TOOLBAR) ? 0x888888U : th.accent;
                            s.fillRect(tx, ty, 2, lineH - 2, caretColor);
                        }
                    }

                    if (i < textIdx) continue; // skipped markers

                    if (i + 1 < start + len && noteText.charAt(i) == '*' && noteText.charAt(i+1) == '*') {
                        isBold = !isBold;
                        if (i + 1 == noteCursor) {
                            if (state == WordState::TOOLBAR || (millis() / 500) % 2 == 0) {
                                uint32_t caretColor = (state == WordState::TOOLBAR) ? 0x888888U : th.accent;
                                s.fillRect(tx, ty, 2, lineH - 2, caretColor);
                            }
                        }
                        i++; 
                        continue;
                    }
                    if (i + 1 < start + len && noteText.charAt(i) == '_' && noteText.charAt(i+1) == '_') {
                        isUnderline = !isUnderline;
                        if (i + 1 == noteCursor) {
                            if (state == WordState::TOOLBAR || (millis() / 500) % 2 == 0) {
                                uint32_t caretColor = (state == WordState::TOOLBAR) ? 0x888888U : th.accent;
                                s.fillRect(tx, ty, 2, lineH - 2, caretColor);
                            }
                        }
                        i++; 
                        continue;
                    }
                    if (i + 2 < start + len && noteText.charAt(i) == '~' && noteText.charAt(i+2) == '~') {
                        char cc = noteText.charAt(i+1);
                        if (cc == 'r') { activeColor = 0xFF0000U; i+=2; continue; }
                        if (cc == 'g') { activeColor = 0x008800U; i+=2; continue; }
                        if (cc == 'b') { activeColor = 0x0000FFU; i+=2; continue; }
                        if (cc == 'k') { activeColor = 0x000000U; i+=2; continue; }
                    }

                    char c = noteText.charAt(i);
                    if (c == '\n') continue;

                    uint32_t color = activeColor;
                    if (hLevel == 1) color = 0x0000FFU; // Blue for H1
                    else if (hLevel == 2) color = 0x0066CCU;
                    else if (hLevel >= 3) color = 0x3399FFU;
                    
                    bool isSelected = false;
                    if (selectionStart != -1 && selectionEnd != -1 && selectionStart != selectionEnd) {
                        int selMin = min(selectionStart, selectionEnd);
                        int selMax = max(selectionStart, selectionEnd);
                        if (i >= selMin && i < selMax) isSelected = true;
                    }

                    uint32_t drawColor = isSelected ? 0xFFFFFFU : color;
                    uint32_t drawBg = isSelected ? 0x0000FFU : paperWhite;
                    
                    s.setTextColor(drawColor, drawBg);
                    s.setCursor(tx, ty);
                    s.print(c);
                    
                    if (isBold || hLevel > 0) {
                        s.setCursor(tx + 1, ty);
                        s.print(c);
                    }
                    if (isUnderline) {
                        s.drawLine(tx, ty + 9, tx + 5, ty + 9, drawColor);
                    }
                    tx += 6;
                }
                
                if (noteCursor == start + len) {
                    if (state == WordState::TOOLBAR || (millis() / 500) % 2 == 0) {
                        uint32_t caretColor = (state == WordState::TOOLBAR) ? 0x888888U : th.accent;
                        s.fillRect(tx, ty, 2, lineH - 2, caretColor);
                    }
                }
            }

            if (state == WordState::EDITING) {
                UI::drawFooter(s, th, "ESC Files  TAB Tools  Shift+Arrows Select");
            } else if (state == WordState::TOOLBAR) {
                int ribbonW = 304;
                int ribbonH = 22;
                s.fillRect(8, 40, ribbonW, ribbonH, th.bgPanel);
                s.drawRect(8, 40, ribbonW, ribbonH, th.accent);
                s.drawRect(9, 41, ribbonW - 2, ribbonH - 2, th.accentDim);

                int count = getToolbarCount();
                if (count > 0) {
                    int itemW = 296 / count;
                    int extra = 296 % count;
                    for (int i = 0; i < count; i++) {
                        int itemX = 12 + i * itemW + min(i, extra);
                        int itemY = 43;
                        int itemH = 16;
                        bool isSel = (i == toolbarIndex);

                        bool isActive = false;
                        if (toolbarMenuLevel == 0) {
                            if (i == 1) isActive = isCursorBold();
                            if (i == 2) isActive = isCursorUnderline();
                            if (i == 4) isActive = (getCursorHeadingLevel() > 0);
                            if (i == 5) isActive = (getCursorAlign() > 0);
                        } else if (toolbarMenuLevel == 1) {
                            int hl = getCursorHeadingLevel();
                            if (i == 0 && hl == 1) isActive = true;
                            if (i == 1 && hl == 2) isActive = true;
                            if (i == 2 && hl == 3) isActive = true;
                        } else if (toolbarMenuLevel == 2) {
                            int al = getCursorAlign();
                            if (i == 0 && al == 0) isActive = true;
                            if (i == 1 && al == 1) isActive = true;
                            if (i == 2 && al == 2) isActive = true;
                        }

                        if (isSel) {
                            s.fillRect(itemX, itemY, itemW - 4, itemH, th.bgRaised);
                            s.drawRect(itemX, itemY, itemW - 4, itemH, th.accent);
                            s.setTextColor(th.accentBright, th.bgRaised);
                        } else if (isActive) {
                            s.fillRect(itemX, itemY, itemW - 4, itemH, th.accentDim);
                            s.setTextColor(th.bg, th.accentDim);
                        } else {
                            s.setTextColor(th.textPrimary, th.bgPanel);
                        }

                        const char* lbl = getToolbarLabel(i);
                        int tw = strlen(lbl) * 6;
                        s.setCursor(itemX + (itemW - 4 - tw) / 2, itemY + 4);
                        s.print(lbl);
                    }
                }
                UI::drawFooter(s, th, "Left/Right Navigate   ENTER Apply   TAB Return");
            }
        }
    }

    void drawStatus(AppContext* ctx) override {
        auto& is = *ctx->intDisplay;
        const Theme& th = *ctx->theme;
        
        is.fillRect(0, 10, 240, 125, th.bg); 
        
        if (state == WordState::FILE_BROWSER || state == WordState::NAMING_FILE) {
            is.drawLine(10, 85, 230, 85, th.border);

            char timeBuf[8];
            ctx->getTime(timeBuf, sizeof(timeBuf));
            is.setTextColor(th.accentBright, th.bg);
            is.setTextSize(3);
            is.setCursor(75, 26);
            is.print(timeBuf);

            char dateBuf[8];
            ctx->getDateDDMM(dateBuf, sizeof(dateBuf));
            char yearBuf[6];
            ctx->getYear(yearBuf, sizeof(yearBuf));
            is.setTextSize(1);
            is.setTextColor(th.textDim, th.bg);
            is.setCursor(95, 60);
            is.printf("%s.%s", dateBuf, yearBuf);

            is.setTextColor(th.accent, th.bg);
            is.setCursor(20, 96);
            if (state == WordState::FILE_BROWSER) {
                is.print("DOCUMENTS LIBRARY");
                is.setTextColor(th.textFaint, th.bg);
                is.setCursor(20, 110);
                is.printf("Files cataloged: %d", fileList.size());
            } else {
                is.print("NAMING NEW DOCUMENT");
                is.setTextColor(th.textFaint, th.bg);
                is.setCursor(20, 110);
                is.print("Type title on external display");
            }
        } else {
            // EDITING or TOOLBAR - Distraction-Free Stats Panel
            is.drawLine(142, 20, 142, 120, th.border);

            // Left side stats
            int words = getWordCount();
            int chars = noteText.length();

            is.setTextColor(th.textFaint, th.bg);
            is.setTextSize(1);
            is.setCursor(12, 22);
            is.print("WORD COUNT");
            
            is.setTextColor(th.accentBright, th.bg);
            is.setTextSize(3);
            is.setCursor(12, 36);
            is.printf("%d", words);
            
            is.setTextColor(th.textDim, th.bg);
            is.setTextSize(1);
            is.setCursor(12, 68);
            is.printf("Chars: %d", chars);

            unsigned long flowMs = millis() - sessionStartTime;
            int flowMins = flowMs / 60000;
            int flowSecs = (flowMs % 60000) / 1000;

            is.setTextColor(th.accent, th.bg);
            is.setTextSize(1);
            is.setCursor(12, 88);
            is.printf("Flow: %dm %02ds", flowMins, flowSecs);

            is.drawRect(12, 108, 118, 8, th.border);
            int gaugeW = (114 * chars) / 3000;
            if (gaugeW > 114) gaugeW = 114;
            if (gaugeW < 0) gaugeW = 0;
            is.fillRect(14, 110, gaugeW, 4, th.accent);

            // Right side state indicator
            is.fillRect(148, 20, 82, 100, th.bgPanel);
            
            if (noteSaved) {
                is.drawRect(148, 20, 82, 100, th.indicator);
                
                // Draw Lock
                is.fillRect(180, 50, 18, 14, th.indicator);
                is.drawRect(183, 44, 12, 6, th.indicator);
                is.fillRect(188, 54, 2, 4, th.bgPanel); // Draw hole against bgPanel

                is.setTextColor(th.indicator, th.bgPanel);
                is.setTextSize(1);
                is.setCursor(174, 90);
                is.print("SAVED");
            } else {
                is.drawRect(148, 20, 82, 100, th.accent);

                // Draw Pencil
                is.drawLine(180, 62, 198, 44, th.accent);
                is.drawLine(181, 62, 199, 44, th.accent);
                is.drawLine(180, 61, 198, 43, th.accent);
                is.fillRect(178, 62, 2, 2, th.accentDim);

                is.setTextColor(th.accent, th.bgPanel);
                is.setTextSize(1);
                is.setCursor(165, 90);
                is.print("MODIFIED");
            }
        }
    }

    bool handleInput(AppContext* ctx, char key, bool isDel, bool isEnter) override {
        bool isTab = M5Cardputer.Keyboard.keysState().tab;
        bool isOpt = M5Cardputer.Keyboard.keysState().opt;
        bool isShift = M5Cardputer.Keyboard.keysState().shift;

        if (state == WordState::FILE_BROWSER) {
            if (key == 's' || key == '.' || key == '>' || (isOpt && key == '.')) {
                if (fileSelectedIndex + 2 < (int)fileList.size()) fileSelectedIndex += 2;
            } else if (key == 'w' || key == ';' || key == ':' || (isOpt && key == ';')) {
                if (fileSelectedIndex - 2 >= 0) fileSelectedIndex -= 2;
            } else if (key == 'a' || key == ',' || key == '<' || (isOpt && key == ',')) {
                if (fileSelectedIndex > 0) fileSelectedIndex--;
            } else if (key == 'd' || key == '/' || key == '?' || (isOpt && key == '/')) {
                if (fileSelectedIndex + 1 < (int)fileList.size()) fileSelectedIndex++;
            }
            while (fileSelectedIndex < fileScrollOffset) {
                fileScrollOffset -= 2;
            }
            while (fileSelectedIndex >= fileScrollOffset + 6) {
                fileScrollOffset += 2;
            }
            if (fileScrollOffset < 0) fileScrollOffset = 0;

            if (isEnter) {
                if (fileSelectedIndex == 0) {
                    currentFile = "";
                    noteText = "";
                    noteCursor = 0;
                    noteSaved = true;
                    selectionStart = -1;
                    selectionEnd = -1;
                    state = WordState::NAMING_FILE;
                    updateLayout();
                    updateDesiredCol();
                } else {
                    loadNote(ctx, fileList[fileSelectedIndex].name);
                    state = WordState::EDITING;
                }
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CONFIRM);
            } else if (isDel && fileSelectedIndex > 0) {
                String path = notesDir + "/" + fileList[fileSelectedIndex].name;
                if (SD.remove(path)) {
                    loadFileList(ctx);
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::ALERT);
                } else {
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::ERROR);
                }
            } else if (key == 27 || key == '`') { 
                return false; 
            }
            return true;
        }

        if (state == WordState::NAMING_FILE) {
            if (key == 27 || key == '`') { 
                state = WordState::FILE_BROWSER;
            } else if (isEnter) {
                if (currentFile == "") {
                    currentFile = notesDir + "/Doc_" + ctx->getCurrentDateDDMM() + "_" + String(millis() % 10000) + ".md";
                } else {
                    if (!currentFile.endsWith(".md") && !currentFile.endsWith(".txt")) {
                        currentFile += ".md";
                    }
                    currentFile = notesDir + "/" + currentFile;
                }
                state = WordState::EDITING;
                sessionStartTime = millis();
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CONFIRM);
            } else if (isDel) {
                if (currentFile.length() > 0) {
                    currentFile.remove(currentFile.length() - 1);
                }
            } else if (key >= 32 && key <= 126) {
                if (currentFile.length() < 20 && key != '/' && key != '\\') { 
                    currentFile += (char)key;
                }
            }
            return true;
        }
        
        if (state == WordState::TOOLBAR) {
            int count = getToolbarCount();
            if (key == 'a' || key == '<' || key == ',' || (isOpt && key == ',')) {
                if (count > 0) toolbarIndex = (toolbarIndex - 1 + count) % count;
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
            } else if (key == 'd' || key == '?' || key == '/' || (isOpt && key == '/')) {
                if (count > 0) toolbarIndex = (toolbarIndex + 1) % count;
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
            } else if (isEnter) {
                if (toolbarMenuLevel == 0) {
                    if (toolbarIndex == 0) saveNote(ctx);
                    else if (toolbarIndex == 1) {
                        if (selectionStart != -1 && selectionEnd != -1 && selectionStart != selectionEnd) {
                            int selMin = min(selectionStart, selectionEnd);
                            int selMax = max(selectionStart, selectionEnd);
                            noteText = noteText.substring(0, selMin) + "**" + noteText.substring(selMin, selMax) + "**" + noteText.substring(selMax);
                            noteCursor = selMax + 4;
                            clearSelection();
                            updateLayout();
                        } else {
                            insertAtCursor(ctx, "**"); 
                        }
                    } 
                    else if (toolbarIndex == 2) {
                        if (selectionStart != -1 && selectionEnd != -1 && selectionStart != selectionEnd) {
                            int selMin = min(selectionStart, selectionEnd);
                            int selMax = max(selectionStart, selectionEnd);
                            noteText = noteText.substring(0, selMin) + "__" + noteText.substring(selMin, selMax) + "__" + noteText.substring(selMax);
                            noteCursor = selMax + 4;
                            clearSelection();
                            updateLayout();
                        } else {
                            insertAtCursor(ctx, "__"); 
                        }
                    } 
                    else if (toolbarIndex == 3) { toolbarMenuLevel = 3; toolbarIndex = 0; SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK); return true; } // Color...
                    else if (toolbarIndex == 4) { toolbarMenuLevel = 1; toolbarIndex = 0; SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK); return true; } // Head...
                    else if (toolbarIndex == 5) { toolbarMenuLevel = 2; toolbarIndex = 0; SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK); return true; } // Align...
                } else if (toolbarMenuLevel == 1) { // Heading
                    if (toolbarIndex >= 0 && toolbarIndex <= 2) {
                        int hLevels[3] = {1, 2, 3};
                        int level = hLevels[toolbarIndex];
                        int lineStart = 0;
                        for (int i=noteCursor-1; i>=0; i--) {
                            if (noteText.charAt(i) == '\n') { lineStart = i + 1; break; }
                        }
                        int existing = 0;
                        for(int i = lineStart; i < noteText.length(); i++) {
                            if (noteText.charAt(i) == '#') existing++;
                            else if (noteText.charAt(i) == ' ' && existing > 0) { existing++; break; }
                            else { existing = 0; break; }
                        }
                        if (existing > 0) {
                            noteText.remove(lineStart, existing);
                            noteCursor -= existing;
                        }
                        if (getCursorHeadingLevel() != level) { // Toggle off if already that level
                            String hStr = "";
                            for(int i=0; i<level; i++) hStr += "#";
                            hStr += " ";
                            noteText = noteText.substring(0, lineStart) + hStr + noteText.substring(lineStart);
                            noteCursor += hStr.length();
                        }
                        updateLayout();
                    }
                    toolbarMenuLevel = 0; // reset
                    if (toolbarIndex == 3) { toolbarIndex = 4; SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK); return true; } // Back pressed
                } else if (toolbarMenuLevel == 2) { // Align
                    if (toolbarIndex >= 0 && toolbarIndex <= 2) {
                        int lineStart = 0;
                        for (int i=noteCursor-1; i>=0; i--) {
                            if (noteText.charAt(i) == '\n') { lineStart = i + 1; break; }
                        }
                        if (lineStart + 1 < noteText.length() && (noteText.charAt(lineStart) == '~' || noteText.charAt(lineStart) == '>') && noteText.charAt(lineStart+1) == ' ') {
                            noteText.remove(lineStart, 2);
                            noteCursor -= 2;
                        }
                        if (toolbarIndex == 1) {
                            noteText = noteText.substring(0, lineStart) + "~ " + noteText.substring(lineStart);
                            noteCursor += 2;
                        } else if (toolbarIndex == 2) {
                            noteText = noteText.substring(0, lineStart) + "> " + noteText.substring(lineStart);
                            noteCursor += 2;
                        }
                        updateLayout();
                    }
                    toolbarMenuLevel = 0; // reset
                    if (toolbarIndex == 3) { toolbarIndex = 5; SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK); return true; } // Back pressed
                } else if (toolbarMenuLevel == 3) { // Color
                    if (toolbarIndex >= 0 && toolbarIndex <= 3) {
                        String colorCode = "~k~";
                        if (toolbarIndex == 1) colorCode = "~r~";
                        if (toolbarIndex == 2) colorCode = "~g~";
                        if (toolbarIndex == 3) colorCode = "~b~";
                        
                        if (selectionStart != -1 && selectionEnd != -1 && selectionStart != selectionEnd) {
                            int selMin = min(selectionStart, selectionEnd);
                            int selMax = max(selectionStart, selectionEnd);
                            noteText = noteText.substring(0, selMin) + colorCode + noteText.substring(selMin, selMax) + "~k~" + noteText.substring(selMax);
                            noteCursor = selMax + 6;
                            clearSelection();
                            updateLayout();
                        } else {
                            insertAtCursor(ctx, colorCode); 
                        }
                    }
                    toolbarMenuLevel = 0; // reset
                    if (toolbarIndex == 4) { toolbarIndex = 3; SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK); return true; } // Back pressed
                }

                if (toolbarIndex == 6 && toolbarMenuLevel == 0) {
                    // Close
                }
                toolbarMenuLevel = 0;
                state = WordState::EDITING;
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CONFIRM);
            } else if (isTab || key == 27 || key == '`') { 
                if (toolbarMenuLevel != 0) {
                    toolbarMenuLevel = 0;
                    toolbarIndex = 0;
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
                } else {
                    state = WordState::EDITING;
                }
            }
            return true;
        }

        if (state == WordState::EDITING && isOpt) {
            if (key == 'b' || key == 'B') {
                if (selectionStart != -1 && selectionEnd != -1 && selectionStart != selectionEnd) {
                    int selMin = min(selectionStart, selectionEnd);
                    int selMax = max(selectionStart, selectionEnd);
                    noteText = noteText.substring(0, selMin) + "**" + noteText.substring(selMin, selMax) + "**" + noteText.substring(selMax);
                    noteCursor = selMax + 4;
                    clearSelection();
                } else {
                    insertAtCursor(ctx, "****");
                    noteCursor -= 2;
                }
                noteSaved = false;
                ctx->hasUnsavedWork = true;
                updateLayout();
                updateDesiredCol();
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
                return true;
            }
            if (key == 'u' || key == 'U') {
                if (selectionStart != -1 && selectionEnd != -1 && selectionStart != selectionEnd) {
                    int selMin = min(selectionStart, selectionEnd);
                    int selMax = max(selectionStart, selectionEnd);
                    noteText = noteText.substring(0, selMin) + "__" + noteText.substring(selMin, selMax) + "__" + noteText.substring(selMax);
                    noteCursor = selMax + 4;
                    clearSelection();
                } else {
                    insertAtCursor(ctx, "____");
                    noteCursor -= 2;
                }
                noteSaved = false;
                ctx->hasUnsavedWork = true;
                updateLayout();
                updateDesiredCol();
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
                return true;
            }
            if (key == 'h' || key == 'H') {
                int lineStart = 0;
                for (int i = noteCursor - 1; i >= 0; i--) {
                    if (noteText.charAt(i) == '\n') { lineStart = i + 1; break; }
                }
                int existingLevel = 0;
                int existingTextLen = 0;
                for (int i = lineStart; i < noteText.length(); i++) {
                    if (noteText.charAt(i) == '#') existingLevel++;
                    else if (noteText.charAt(i) == ' ' && existingLevel > 0) {
                        existingTextLen = existingLevel + 1;
                        break;
                    } else {
                        existingLevel = 0;
                        break;
                    }
                }
                int nextLevel = (existingLevel + 1) % 4;
                if (existingTextLen > 0) {
                    noteText.remove(lineStart, existingTextLen);
                    noteCursor -= existingTextLen;
                }
                if (nextLevel > 0) {
                    String prefix = "";
                    for (int i = 0; i < nextLevel; i++) prefix += "#";
                    prefix += " ";
                    noteText = noteText.substring(0, lineStart) + prefix + noteText.substring(lineStart);
                    noteCursor += prefix.length();
                }
                noteSaved = false;
                ctx->hasUnsavedWork = true;
                updateLayout();
                updateDesiredCol();
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
                return true;
            }
        }

        if (isTab) {
            state = WordState::TOOLBAR;
            toolbarMenuLevel = 0;
            toolbarIndex = 0;
            return true;
        }
        if (key == 27 || key == '`') { 
            state = WordState::FILE_BROWSER;
            loadFileList(ctx);
            return true; 
        }

        if (M5Cardputer.Keyboard.isKeyPressed(KEY_LEFT_CTRL) && (key == 's' || key == 'S')) {
            saveNote(ctx);
            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CONFIRM);
            return true;
        }

        if (key == '<' || (isOpt && key == ',')) { 
            if (isShift && selectionStart == -1) selectionStart = noteCursor;
            if (isOpt) {
                while (noteCursor > 0 && noteText.charAt(noteCursor - 1) == ' ') noteCursor--;
                while (noteCursor > 0 && noteText.charAt(noteCursor - 1) != ' ' && noteText.charAt(noteCursor - 1) != '\n') noteCursor--;
            } else {
                if (noteCursor > 0) noteCursor--;
            }
            if (isShift) selectionEnd = noteCursor; else clearSelection();
            updateDesiredCol();
            return true;
        } else if (key == '?' || (isOpt && key == '/')) { 
            if (isShift && selectionStart == -1) selectionStart = noteCursor;
            if (isOpt) {
                while (noteCursor < (int)noteText.length() && noteText.charAt(noteCursor) == ' ') noteCursor++;
                while (noteCursor < (int)noteText.length() && noteText.charAt(noteCursor) != ' ' && noteText.charAt(noteCursor) != '\n') noteCursor++;
            } else {
                if (noteCursor < (int)noteText.length()) noteCursor++;
            }
            if (isShift) selectionEnd = noteCursor; else clearSelection();
            updateDesiredCol();
            return true;
        } else if (key == ':' || (isOpt && key == ';')) { 
            if (isShift && selectionStart == -1) selectionStart = noteCursor;
            int r, c;
            getCursorPos(r, c);
            if (r > 0) {
                r--;
                int len = displayLines[r].length;
                if (len > 0 && noteText.charAt(displayLines[r].startIndex + len - 1) == '\n') {
                    len--; 
                }
                c = desiredCol;
                if (c > len) c = len;
                noteCursor = displayLines[r].startIndex + c;
            } else {
                noteCursor = 0;
            }
            if (isShift) selectionEnd = noteCursor; else clearSelection();
            return true;
        } else if (key == '>' || (isOpt && key == '.')) { 
            if (isShift && selectionStart == -1) selectionStart = noteCursor;
            int r, c;
            getCursorPos(r, c);
            if (r < (int)displayLines.size() - 1) {
                r++;
                int len = displayLines[r].length;
                if (len > 0 && noteText.charAt(displayLines[r].startIndex + len - 1) == '\n') {
                    len--;
                }
                c = desiredCol;
                if (c > len) c = len;
                noteCursor = displayLines[r].startIndex + c;
            } else {
                noteCursor = noteText.length();
            }
            if (isShift) selectionEnd = noteCursor; else clearSelection();
            return true;
        }

        if (isDel) {
            if (selectionStart != -1 && selectionEnd != -1 && selectionStart != selectionEnd) {
                int selMin = min(selectionStart, selectionEnd);
                int selMax = max(selectionStart, selectionEnd);
                noteText.remove(selMin, selMax - selMin);
                noteCursor = selMin;
                clearSelection();
                noteSaved = false;
                ctx->hasUnsavedWork = true;
                updateLayout();
                updateDesiredCol();
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
            } else if (noteCursor > 0 && noteText.length() > 0) {
                noteText.remove(noteCursor - 1, 1);
                noteCursor--;
                noteSaved = false;
                ctx->hasUnsavedWork = true;
                updateLayout();
                updateDesiredCol();
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
            } else {
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::ERROR);
            }
        } else if (isEnter) {
            if (selectionStart != -1 && selectionEnd != -1 && selectionStart != selectionEnd) {
                int selMin = min(selectionStart, selectionEnd);
                int selMax = max(selectionStart, selectionEnd);
                noteText.remove(selMin, selMax - selMin);
                noteCursor = selMin;
                clearSelection();
            }
            insertAtCursor(ctx, "\n");
            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
        } else if (key >= 32 && key <= 126) {
            if (noteText.length() < 3000) { 
                if (selectionStart != -1 && selectionEnd != -1 && selectionStart != selectionEnd) {
                    int selMin = min(selectionStart, selectionEnd);
                    int selMax = max(selectionStart, selectionEnd);
                    noteText.remove(selMin, selMax - selMin);
                    noteCursor = selMin;
                    clearSelection();
                }
                insertAtCursor(ctx, String(key));
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
            } else {
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::ERROR);
            }
        }
        return true;
    }
};
