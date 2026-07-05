#pragma once
#include "../App.h"
#include <SD.h>
#include <vector>
#include <algorithm>

enum class FileExplorerState {
    BROWSING,
    INPUT_NEW_FILE,
    INPUT_NEW_DIR,
    INPUT_RENAME,
    CONFIRM_DELETE
};

class FileExplorerApp : public App {
private:
    struct FileEntry {
        String name;
        bool isDir;
        uint32_t size;
    };
    std::vector<FileEntry> files;
    String currentPath = "/";
    int selectedIndex = 0;
    int scrollOffset = 0;

    FileExplorerState state = FileExplorerState::BROWSING;
    String inputText = "";
    String targetName = "";

    void loadDirectory(AppContext* ctx, String path) {
        if (ctx->sdAvailable && ctx->extSprite) {
            auto& s = *ctx->extSprite;
            const Theme& th = *ctx->theme;
            s.fillRect(80, 100, 160, 40, th.bg);
            s.drawRect(80, 100, 160, 40, th.accent);
            s.setTextColor(th.textPrimary);
            s.setTextSize(2);
            s.setCursor(110, 112);
            s.print("Loading...");
            s.pushSprite(0, 0);
        }

        files.clear();
        currentPath = path;
        selectedIndex = 0;
        scrollOffset = 0;

        if (!ctx->sdAvailable) return;

        File dir = SD.open(path);
        if (!dir || !dir.isDirectory()) return;

        if (path != "/") files.push_back({"..", true, 0});

        File file = dir.openNextFile();
        while (file && files.size() < 250) {
            String fname = String(file.name());
            if (!fname.startsWith(".")) {
                files.push_back({fname, file.isDirectory(), (uint32_t)file.size()});
            }
            file = dir.openNextFile();
        }
        
        std::sort(files.begin(), files.end(), [](const FileEntry& a, const FileEntry& b) {
            if (a.name == "..") return true;
            if (b.name == "..") return false;
            if (a.isDir && !b.isDir) return true;
            if (!a.isDir && b.isDir) return false;
            String lowerA = a.name; lowerA.toLowerCase();
            String lowerB = b.name; lowerB.toLowerCase();
            return lowerA < lowerB;
        });
    }

    String getFullPath(String fname) {
        if (fname == "..") {
            int lastSlash = currentPath.lastIndexOf('/');
            if (lastSlash <= 0) return "/";
            return currentPath.substring(0, lastSlash);
        }
        if (currentPath == "/") return "/" + fname;
        return currentPath + "/" + fname;
    }

    String formatSize(uint32_t bytes) {
        if (bytes < 1024) return String(bytes) + " B";
        if (bytes < 1024 * 1024) return String(bytes / 1024) + " KB";
        return String(bytes / (1024 * 1024)) + " MB";
    }

    void handleCRUD(AppContext* ctx) {
        String fullTarget = getFullPath(targetName);
        if (state == FileExplorerState::INPUT_NEW_FILE) {
            String newFile = getFullPath(inputText);
            if (!SD.exists(newFile)) {
                File f = SD.open(newFile, FILE_WRITE);
                if (f) f.close();
                else ctx->showNotification("Create failed");
            } else ctx->showNotification("Exists!");
        } else if (state == FileExplorerState::INPUT_NEW_DIR) {
            String newDir = getFullPath(inputText);
            if (!SD.exists(newDir)) {
                if (!SD.mkdir(newDir)) ctx->showNotification("Mkdir failed");
            } else ctx->showNotification("Exists!");
        } else if (state == FileExplorerState::INPUT_RENAME) {
            String renamedFile = getFullPath(inputText);
            if (!SD.exists(renamedFile)) {
                if (!SD.rename(fullTarget, renamedFile)) ctx->showNotification("Rename failed");
            } else ctx->showNotification("Exists!");
        } else if (state == FileExplorerState::CONFIRM_DELETE) {
            if (targetName != "..") {
                bool isDir = false;
                for(auto& f : files) if(f.name == targetName) { isDir = f.isDir; break; }
                
                if (isDir) {
                    if (!SD.rmdir(fullTarget)) ctx->showNotification("Dir not empty?");
                } else {
                    if (!SD.remove(fullTarget)) ctx->showNotification("Delete failed");
                }
            }
        }
        
        state = FileExplorerState::BROWSING;
        loadDirectory(ctx, currentPath);
    }

public:
    void setup(AppContext* ctx, String args) override {
        state = FileExplorerState::BROWSING;
        loadDirectory(ctx, "/");
    }

    int drawPill(LGFX_Sprite& s, int x, int y, const char* label, const Theme& th, bool highlight = false) {
        int w = strlen(label) * 6 + 8;
        uint16_t bg = highlight ? th.accent : th.selectBg;
        uint16_t textCol = highlight ? th.bg : th.textPrimary;
        s.fillRoundRect(x, y, w, 14, 4, bg);
        s.setTextColor(textCol);
        s.setTextSize(1);
        s.setCursor(x + 4, y + 3);
        s.print(label);
        return w + 4;
    }

    void draw(AppContext* ctx) override {
        auto& s = *ctx->extSprite;
        const Theme& th = *ctx->theme;
        s.fillScreen(th.bg);

        // Header
        { char tb[8]; ctx->getTime(tb, sizeof(tb)); UI::drawHeader(s, th, ctx->userName.c_str(), tb); }

        // Path display
        UI::drawSectionLabel(s, 8, 26, "FILE EXPLORER", th);

        s.setTextColor(th.textDim);
        s.setTextSize(1);
        s.setCursor(110, 26);
        s.print(currentPath.c_str());

        // Item count
        s.setTextColor(th.textFaint);
        s.setCursor(280, 26);
        s.printf("[%d]", files.size());

        UI::drawSeparator(s, 4, 38, 312, th);

        if (!ctx->sdAvailable) {
            s.setTextColor(th.danger);
            s.setTextSize(1);
            s.setCursor(20, 60);
            s.print("SD Card Not Found");
            UI::drawLED(s, 140, 60, th.danger);
            UI::drawFooter(s, th, "ESC Back");
            return;
        }

        if (files.empty()) {
            s.setTextColor(th.textDim);
            s.setCursor(20, 60);
            s.print("Empty directory");
        }

        // ---- File listing ----
        const int ITEMS_PER_PAGE = 8;
        const int ROW_H = 18;
        const int LIST_TOP = 42;

        for (int i = 0; i < ITEMS_PER_PAGE; i++) {
            int idx = scrollOffset + i;
            if (idx >= (int)files.size()) break;

            int y = LIST_TOP + i * ROW_H;

            if (idx == selectedIndex) {
                s.fillRect(6, y, 308, ROW_H - 2, th.selectBg);
                s.drawRect(6, y, 308, ROW_H - 2, th.accent);
                s.fillRect(6, y + 2, 2, ROW_H - 6, th.accent);
            }

            s.setTextSize(1);
            if (files[idx].isDir) {
                s.setTextColor(idx == selectedIndex ? th.accent : th.accentDim);
                s.setCursor(12, y + 4);
                s.print("[DIR]");
            } else {
                s.setTextColor(idx == selectedIndex ? th.textDim : th.textFaint);
                s.setCursor(12, y + 4);
                s.print("[---]");
            }

            s.setTextColor(idx == selectedIndex ? th.textPrimary : th.textDim);
            s.setCursor(46, y + 4);
            s.print(files[idx].name.c_str());
            
            if (!files[idx].isDir) {
                s.setTextColor(idx == selectedIndex ? th.textDim : th.textFaint);
                String sizeStr = formatSize(files[idx].size);
                s.setCursor(308 - (sizeStr.length() * 6), y + 4);
                s.print(sizeStr.c_str());
            }
        }

        // ---- Scrollbar ----
        if ((int)files.size() > ITEMS_PER_PAGE) {
            int sbH = ITEMS_PER_PAGE * ROW_H;
            int thumbH = max(8, sbH * ITEMS_PER_PAGE / (int)files.size());
            int thumbY = LIST_TOP + (sbH - thumbH) * scrollOffset / max(1, (int)files.size() - ITEMS_PER_PAGE);
            s.drawRect(314, LIST_TOP, 4, sbH, th.border);
            s.fillRect(315, thumbY, 2, thumbH, th.accent);
        }

        // Overlays
        if (state != FileExplorerState::BROWSING) {
            UI::drawRecessedPanel(s, 20, 80, 280, 70, th);
            s.setTextColor(th.accentBright);
            s.setTextSize(1);
            s.setCursor(30, 90);
            
            if (state == FileExplorerState::INPUT_NEW_FILE) {
                s.print("CREATE NEW FILE");
            } else if (state == FileExplorerState::INPUT_NEW_DIR) {
                s.print("CREATE NEW DIR");
            } else if (state == FileExplorerState::INPUT_RENAME) {
                s.printf("RENAME: %s", targetName.c_str());
            } else if (state == FileExplorerState::CONFIRM_DELETE) {
                s.setTextColor(th.danger);
                s.printf("DELETE: %s?", targetName.c_str());
            }

            if (state == FileExplorerState::CONFIRM_DELETE) {
                s.setTextColor(th.textPrimary);
                s.setCursor(30, 115);
                s.print("Press ENTER to delete");
                s.setCursor(30, 130);
                s.print("Press ESC to cancel");
            } else {
                UI::drawRecessedPanel(s, 28, 105, 264, 20, th);
                s.setTextColor(th.textPrimary);
                s.setCursor(34, 111);
                s.print(inputText.c_str());
                if ((millis() / 500) % 2 == 0) {
                    s.fillRect(34 + inputText.length() * 6, 109, 6, 12, th.accent);
                }
            }
        }

        // Footer & Hotkey Menu
        if (state == FileExplorerState::BROWSING) {
            UI::drawRecessedPanel(s, 6, 190, 308, 42, th);
            s.setTextColor(th.accentBright);
            s.setTextSize(1);
            s.setCursor(14, 196);
            s.print("ACTIONS:");
            
            int px = 14;
            int py = 212;
            px += drawPill(s, px, py, "1", th, true);
            s.setTextColor(th.textPrimary); s.setCursor(px, py+3); s.print("File "); px += 33;
            
            px += drawPill(s, px, py, "2", th, true);
            s.setTextColor(th.textPrimary); s.setCursor(px, py+3); s.print("Dir "); px += 27;

            px += drawPill(s, px, py, "3", th, true);
            s.setTextColor(th.textPrimary); s.setCursor(px, py+3); s.print("Ren "); px += 27;

            px += drawPill(s, px, py, "4", th, true);
            s.setTextColor(th.textPrimary); s.setCursor(px, py+3); s.print("Del");

            px = 166;
            py = 194;
            px += drawPill(s, px, py, "ENTER", th);
            s.setTextColor(th.textFaint); s.setCursor(px, py+3); s.print("Open  "); px += 38;
            
            px += drawPill(s, px, py, "ESC", th);
            s.setTextColor(th.textFaint); s.setCursor(px, py+3); s.print("Back");

        } else if (state == FileExplorerState::CONFIRM_DELETE) {
            s.fillRect(0, 224, 320, 16, th.bg); // Clear footer area
            int px = 10;
            px += drawPill(s, px, 225, "ENTER", th, true);
            s.setTextColor(th.textPrimary); s.setCursor(px, 228); s.print("Confirm  "); px += 55;
            
            px += drawPill(s, px, 225, "ESC", th);
            s.setTextColor(th.textPrimary); s.setCursor(px, 228); s.print("Cancel");
        } else {
            s.fillRect(0, 224, 320, 16, th.bg);
            int px = 10;
            s.setTextColor(th.textFaint); s.setCursor(px, 228); s.print("Type name "); px += 62;
            
            px += drawPill(s, px, 225, "ENTER", th, true);
            s.setTextColor(th.textPrimary); s.setCursor(px, 228); s.print("Confirm  "); px += 55;
            
            px += drawPill(s, px, 225, "ESC", th);
            s.setTextColor(th.textPrimary); s.setCursor(px, 228); s.print("Cancel");
        }
    }

    void drawStatus(AppContext* ctx) override {
        auto& is = *ctx->intDisplay;
        const Theme& th = *ctx->theme;
        
        is.fillRect(0, 10, 240, 125, th.bg);

        if (files.empty()) {
            is.setTextColor(th.textDim);
            is.setTextSize(3);
            is.setCursor(20, 50);
            is.print("Empty Dir");
            return;
        }

        auto& f = files[selectedIndex];
        
        // Draw large icon based on type
        int iconX = 20;
        int iconY = 25;
        if (f.name == "..") {
            is.fillTriangle(iconX+40, iconY, iconX, iconY+25, iconX+40, iconY+50, th.accentDim);
        } else if (f.isDir) {
            is.fillRect(iconX, iconY+10, 60, 40, th.accent); // folder body
            is.fillRect(iconX, iconY, 25, 10, th.accent); // folder tab
        } else {
            String lowerName = f.name;
            lowerName.toLowerCase();
            uint16_t fileColor = th.textPrimary;
            if (lowerName.endsWith(".md") || lowerName.endsWith(".txt")) fileColor = th.textPrimary;
            else if (lowerName.endsWith(".json") || lowerName.endsWith(".ini") || lowerName.endsWith(".conf")) fileColor = th.accentDim;
            else fileColor = th.textFaint;
            
            is.fillRect(iconX, iconY, 45, 55, fileColor); // file document
            is.fillTriangle(iconX+30, iconY, iconX+45, iconY, iconX+45, iconY+15, th.bg); // folded corner
        }
        
        is.setTextSize(2);
        is.setTextColor(th.accentBright);
        
        // Ensure name fits
        String dispName = f.name;
        if (dispName.length() > 10) dispName = dispName.substring(0, 10) + "..";
        is.setCursor(95, 25);
        is.print(dispName.c_str());

        if (!f.isDir && f.name != "..") {
            is.setTextSize(3); // LARGE CRITICAL DIGITS
            is.setTextColor(th.textPrimary);
            is.setCursor(95, 50);
            is.print(formatSize(f.size).c_str());
        } else if (f.isDir && f.name != "..") {
            is.setTextSize(2);
            is.setTextColor(th.textDim);
            is.setCursor(95, 55);
            is.print("Folder");
        } else {
            is.setTextSize(2);
            is.setTextColor(th.textDim);
            is.setCursor(95, 55);
            is.print("Up Dir");
        }
        
        if (state != FileExplorerState::BROWSING) {
            is.fillRect(0, 90, 240, 45, th.accentDim);
            is.setTextColor(th.bg);
            is.setTextSize(3); // LARGE TEXT FOR OVERLAYS
            is.setCursor(10, 102);
            if (state == FileExplorerState::CONFIRM_DELETE) {
                is.print("DELETE?");
            } else {
                is.print("INPUT MODE");
            }
        }
    }

    bool handleInput(AppContext* ctx, char key, bool isDel, bool isEnter) override {
        if (!ctx->sdAvailable) return false;

        if (state == FileExplorerState::BROWSING) {
            if (files.empty()) {
                if (key == '1') { state = FileExplorerState::INPUT_NEW_FILE; inputText = ""; targetName = ""; return true; }
                if (key == '2') { state = FileExplorerState::INPUT_NEW_DIR; inputText = ""; targetName = ""; return true; }
                return false;
            }

            if (key == 's' || key == '.') {
                if (selectedIndex < (int)files.size() - 1) {
                    selectedIndex++;
                    if (selectedIndex >= scrollOffset + 8) scrollOffset++;
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                }
            } else if (key == 'w' || key == ';') {
                if (selectedIndex > 0) {
                    selectedIndex--;
                    if (selectedIndex < scrollOffset) scrollOffset--;
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                }
            } else if (key == '1') {
                state = FileExplorerState::INPUT_NEW_FILE;
                inputText = ""; targetName = "";
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
            } else if (key == '2') {
                state = FileExplorerState::INPUT_NEW_DIR;
                inputText = ""; targetName = "";
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
            } else if (key == '3') {
                if (files[selectedIndex].name != "..") {
                    state = FileExplorerState::INPUT_RENAME;
                    inputText = files[selectedIndex].name;
                    targetName = files[selectedIndex].name;
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
                }
            } else if (key == '4' || isDel) {
                if (files[selectedIndex].name != "..") {
                    state = FileExplorerState::CONFIRM_DELETE;
                    targetName = files[selectedIndex].name;
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::ALERT);
                }
            } else if (isEnter) {
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
                auto& f = files[selectedIndex];
                if (f.isDir) {
                    loadDirectory(ctx, getFullPath(f.name));
                } else {
                    String fullPath = getFullPath(f.name);
                    String lowerName = f.name;
                    lowerName.toLowerCase();

                    if (lowerName.endsWith(".md") || lowerName.endsWith(".txt")) {
                        ctx->requestAppSwitch(AppType::NOTEPAD, fullPath);
                    } else {
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::ERROR);
                        ctx->showNotification("Unknown file type");
                    }
                }
            }
            return false;
        } else {
            if (key == 27 || key == '`') {
                state = FileExplorerState::BROWSING;
                return true;
            }

            if (state == FileExplorerState::CONFIRM_DELETE) {
                if (isEnter) {
                    handleCRUD(ctx);
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CONFIRM);
                }
            } else {
                if (isEnter) {
                    if (inputText.length() > 0) {
                        handleCRUD(ctx);
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CONFIRM);
                    }
                } else if (isDel) {
                    if (inputText.length() > 0) {
                        inputText.remove(inputText.length() - 1);
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
                    }
                } else if (key >= 32 && key <= 126) {
                    if (inputText.length() < 30 && key != '/' && key != '\\') {
                        inputText += (char)key;
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
                    }
                }
            }
            return true;
        }
        return false;
    }
};

