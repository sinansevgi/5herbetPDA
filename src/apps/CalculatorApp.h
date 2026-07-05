#pragma once
#include "../App.h"

class CalculatorApp : public App {
private:
    String calcInput = "";
    float calcResult = 0;
    char calcOp = ' ';
    float calcLeft = 0;
    bool calcNewNumber = true;

    String tapeHistory[5];
    int tapeCount = 0;

    int cursorX = 0;
    int cursorY = 4; // Start at '0'

    const char* gridKeys[5][4] = {
        {"C", "DEL", "", "/"},
        {"7", "8",   "9",  "*"},
        {"4", "5",   "6",  "-"},
        {"1", "2",   "3",  "+"},
        {"0", ".",   "=", "="}
    };

    void calculateResult() {
        float right = calcInput.toFloat();
        if (calcOp == '+') calcResult = calcLeft + right;
        else if (calcOp == '-') calcResult = calcLeft - right;
        else if (calcOp == '*') calcResult = calcLeft * right;
        else if (calcOp == '/') calcResult = (right != 0) ? calcLeft / right : 0;

        char hbuf[40];
        snprintf(hbuf, sizeof(hbuf), "%.4g %c %.4g = %.4g", calcLeft, calcOp, right, calcResult);
        if (tapeCount < 5) {
            tapeHistory[tapeCount++] = String(hbuf);
        } else {
            for (int i = 0; i < 4; i++) tapeHistory[i] = tapeHistory[i+1];
            tapeHistory[4] = String(hbuf);
        }

        calcInput = String(calcResult);
        calcNewNumber = true;
    }

    void handleGridAction(AppContext* ctx, const char* action) {
        if (strcmp(action, "") == 0) return;

        char key = action[0];
        if (strlen(action) == 1 && ((key >= '0' && key <= '9') || key == '.')) {
            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
            if (calcNewNumber) {
                calcInput = "";
                calcNewNumber = false;
            }
            if (calcInput.length() < 15) calcInput += key;
        } else if (strlen(action) == 1 && (key == '+' || key == '-' || key == '*' || key == '/')) {
            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
            calcLeft = calcInput.toFloat();
            calcOp = key;
            calcNewNumber = true;
        } else if (strcmp(action, "=") == 0) {
            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CONFIRM);
            if (calcOp != ' ') {
                calculateResult();
                calcOp = ' ';
            }
        } else if (strcmp(action, "C") == 0) {
            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::ERROR);
            calcInput = "";
            tapeCount = 0;
            calcLeft = 0;
            calcResult = 0;
            calcOp = ' ';
            calcNewNumber = true;
        } else if (strcmp(action, "DEL") == 0) {
            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
            if (calcInput.length() > 0 && !calcNewNumber) {
                calcInput.remove(calcInput.length() - 1);
            }
        }
    }

public:
    void setup(AppContext* ctx, String args) override {}

    void draw(AppContext* ctx) override {
        auto& s = *ctx->extSprite;
        const Theme& th = *ctx->theme;
        s.fillScreen(th.bg);

        // Header
        { char tb[8]; ctx->getTime(tb, sizeof(tb)); UI::drawHeader(s, th, ctx->userName.c_str(), tb); }

        // App title
        UI::drawSectionLabel(s, 8, 26, "CALCULATOR", th);

        // LCD Display
        UI::drawRecessedPanel(s, 8, 40, 304, 50, th);

        // Current Operator small indicator inside LCD
        if (calcOp != ' ') {
            s.setTextColor(th.textDim);
            s.setTextSize(1);
            s.setCursor(14, 44);
            s.printf("OP: %c", calcOp);
        }

        s.setTextColor(th.accent);
        s.setTextSize(4);
        s.setTextDatum(MR_DATUM);
        s.drawString(calcInput == "" ? "0" : calcInput.c_str(), 306, 65);
        s.setTextDatum(TL_DATUM);

        UI::drawSeparator(s, 8, 96, 304, th);

        // Grid Drawing
        int startX = 19;
        int startY = 104;
        int boxW = 60;
        int boxH = 22;
        int paddingX = 14;
        int paddingY = 2;

        s.setTextSize(2);
        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 4; col++) {
                const char* label = gridKeys[row][col];
                if (strcmp(label, "") == 0) continue; // Skip empty buttons if any

                int x = startX + col * (boxW + paddingX);
                int y = startY + row * (boxH + paddingY);
                
                // Make the "=" button double width if it's placed that way
                int currentBoxW = boxW;
                if (row == 4 && col == 2) {
                    currentBoxW = boxW * 2 + paddingX;
                }
                if (row == 4 && col == 3) {
                    continue; // Skip the second half of the double width "="
                }

                bool isSelected = (cursorX == col && cursorY == row);
                // Also highlight if cursor is on the hidden 2nd half of the "=" button
                if (row == 4 && col == 2 && cursorX == 3 && cursorY == 4) isSelected = true;

                if (isSelected) {
                    s.fillRect(x, y, currentBoxW, boxH, th.accent);
                    s.setTextColor(th.bg);
                } else {
                    s.drawRect(x, y, currentBoxW, boxH, th.border);
                    s.setTextColor(th.textPrimary);
                }

                int textW = strlen(label) * 12; // size 2 approx width
                s.setCursor(x + (currentBoxW - textW) / 2, y + 4);
                s.print(label);
            }
        }

        UI::drawFooter(s, th, "Arrows: Move   ENTER: Select   ESC: Back");
    }

    void drawStatus(AppContext* ctx) override {
        auto& is = *ctx->intDisplay;
        const Theme& th = *ctx->theme;
        is.fillScreen(th.bg);

        // Memory Register Display
        is.setTextColor(th.accent);
        is.setTextSize(2);
        is.setCursor(10, 10);
        is.printf("MEM: %.4g", calcLeft);

        // Calculations Paper Tape
        is.setTextColor(th.textDim);
        is.setTextSize(1);
        is.setCursor(10, 40);
        is.print("--- HISTORY TAPE ---");
        for (int i = 0; i < tapeCount; i++) {
            is.setCursor(10, 56 + i * 14);
            is.print(tapeHistory[i].c_str());
        }
    }

    bool handleInput(AppContext* ctx, char key, bool isDel, bool isEnter) override {
        // Map keyboard arrows (Cardputer specific mappings)
        if (key == 'a' || key == ',') { // Left
            if (cursorX > 0) cursorX--;
            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
        } else if (key == 'd' || key == '/') { // Right
            if (cursorX < 3) cursorX++;
            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
        } else if (key == 'w' || key == ';') { // Up
            if (cursorY > 0) cursorY--;
            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
        } else if (key == 's' || key == '.') { // Down
            if (cursorY < 4) cursorY++;
            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
        } else if (isEnter) {
            handleGridAction(ctx, gridKeys[cursorY][cursorX]);
        } else if (isDel) {
            handleGridAction(ctx, "DEL");
        } else if ((key >= '0' && key <= '9') || key == '.' || key == '+' || key == '-' || key == '*' || key == '/') {
            // Allow physical keyboard typing as well
            char action[2] = {key, '\0'};
            handleGridAction(ctx, action);
        } else if (key == '=') {
            handleGridAction(ctx, "=");
        }
        return false;
    }
};
