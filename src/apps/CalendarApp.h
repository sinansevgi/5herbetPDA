#pragma once
#include "../App.h"
#include <vector>
#include <ArduinoJson.h>
#include <SD.h>

class CalendarApp : public App {
public:
    enum class Tab {
        OVERVIEW = 0,
        PLANNER,
        TASKS,
        NOTES,
        HABITS
    };

    enum class InputMode {
        NONE,
        TASK_TITLE,
        TASK_DATE,
        NOTE_TITLE,
        NOTE_CONTENT,
        GOAL_TITLE,
        EVENT_TITLE
    };

    struct Task {
        String id;
        String title;
        int level; 
        int priority; 
        String deadline;
        bool completed;
        std::vector<String> tags;
    };

    struct Event {
        String title;
        String date; // YYYY-MM-DD
        String type;
    };

    struct Note {
        String title;
        String content;
        uint16_t color;
    };

    struct Habit {
        String name;
        int streak;
        bool doneToday;
    };

    struct Goal {
        String title;
        int progressPct;
    };

private:
    Tab currentTab = Tab::OVERVIEW;
    bool isTabFocused = true;
    InputMode inputMode = InputMode::NONE;
    String inputBuffer = "";
    String tempTaskTitle = "";
    String tempDateDefault = "";
    
    // Data Models
    std::vector<Task> tasks;
    std::vector<Event> events;
    std::vector<Note> notes;
    std::vector<Habit> habits;
    std::vector<Goal> goals;

    // View state
    int selectedTaskIdx = 0;
    int taskScroll = 0;
    
    int selectedNoteIdx = 0;
    int noteScroll = 0;

    int selectedHabitIdx = 0;
    int selectedGoalIdx = -1;

    // Planner state
    int currentMonth = 7;
    int currentYear = 2026;
    int selectedDay = 4;
    bool plannerViewDay = false;

    // File paths
    const String dirPath = "/5herbetPDA/organizer";
    const String dataPath = "/5herbetPDA/organizer/data.json";
    bool sdReady = false;

    int daysInMonth(int m, int y) {
        if (m==2) return (y%4==0 && (y%100!=0 || y%400==0)) ? 29 : 28;
        if (m==4 || m==6 || m==9 || m==11) return 30;
        return 31;
    }

    int dayOfWeek(int y, int m, int d) {
        static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
        y -= m < 3;
        int dow = (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7; 
        return (dow == 0) ? 6 : dow - 1;
    }

    void saveData(AppContext* ctx) {
        if(!ctx->sdAvailable) return;
        if(!SD.exists(dirPath)) {
            SD.mkdir(dirPath);
        }

        JsonDocument doc;
        
        JsonArray tArr = doc["tasks"].to<JsonArray>();
        for(auto& t : tasks) {
            JsonObject obj = tArr.add<JsonObject>();
            obj["title"] = t.title;
            obj["level"] = t.level;
            obj["priority"] = t.priority;
            obj["deadline"] = t.deadline;
            obj["completed"] = t.completed;
        }

        JsonArray eArr = doc["events"].to<JsonArray>();
        for(auto& e : events) {
            JsonObject obj = eArr.add<JsonObject>();
            obj["title"] = e.title;
            obj["date"] = e.date;
            obj["type"] = e.type;
        }

        JsonArray nArr = doc["notes"].to<JsonArray>();
        for(auto& n : notes) {
            JsonObject obj = nArr.add<JsonObject>();
            obj["title"] = n.title;
            obj["content"] = n.content;
            obj["color"] = n.color;
        }

        JsonArray hArr = doc["habits"].to<JsonArray>();
        for(auto& h : habits) {
            JsonObject obj = hArr.add<JsonObject>();
            obj["name"] = h.name;
            obj["streak"] = h.streak;
            obj["doneToday"] = h.doneToday;
        }

        JsonArray gArr = doc["goals"].to<JsonArray>();
        for(auto& g : goals) {
            JsonObject obj = gArr.add<JsonObject>();
            obj["title"] = g.title;
            obj["progressPct"] = g.progressPct;
        }

        File file = SD.open(dataPath, FILE_WRITE);
        if(file) {
            serializeJson(doc, file);
            file.close();
            ctx->showNotification("Data Saved");
        }
    }

    void loadData(AppContext* ctx) {
        if(!ctx->sdAvailable) {
            loadMockData(ctx);
            return;
        }
        sdReady = true;

        if(SD.exists(dataPath)) {
            File file = SD.open(dataPath, FILE_READ);
            if(file) {
                JsonDocument doc;
                DeserializationError err = deserializeJson(doc, file);
                file.close();

                if(!err) {
                    tasks.clear();
                    JsonArray tArr = doc["tasks"].as<JsonArray>();
                    for(JsonObject obj : tArr) {
                        Task t;
                        t.title = obj["title"].as<String>();
                        t.level = obj["level"] | 0;
                        t.priority = obj["priority"] | 0;
                        t.deadline = obj["deadline"].as<String>();
                        t.completed = obj["completed"] | false;
                        tasks.push_back(t);
                    }

                    events.clear();
                    JsonArray eArr = doc["events"].as<JsonArray>();
                    for(JsonObject obj : eArr) {
                        Event e;
                        e.title = obj["title"].as<String>();
                        e.date = obj["date"].as<String>();
                        e.type = obj["type"].as<String>();
                        events.push_back(e);
                    }

                    notes.clear();
                    JsonArray nArr = doc["notes"].as<JsonArray>();
                    for(JsonObject obj : nArr) {
                        Note n;
                        n.title = obj["title"].as<String>();
                        n.content = obj["content"].as<String>();
                        n.color = obj["color"] | ctx->theme->accent;
                        notes.push_back(n);
                    }

                    habits.clear();
                    JsonArray hArr = doc["habits"].as<JsonArray>();
                    for(JsonObject obj : hArr) {
                        Habit h;
                        h.name = obj["name"].as<String>();
                        h.streak = obj["streak"] | 0;
                        h.doneToday = obj["doneToday"] | false;
                        habits.push_back(h);
                    }

                    goals.clear();
                    JsonArray gArr = doc["goals"].as<JsonArray>();
                    for(JsonObject obj : gArr) {
                        Goal g;
                        g.title = obj["title"].as<String>();
                        g.progressPct = obj["progressPct"] | 0;
                        goals.push_back(g);
                    }
                    return;
                }
            }
        }
        
        loadMockData(ctx);
        saveData(ctx);
    }

    void loadMockData(AppContext* ctx) {
        const Theme& th = *ctx->theme;
        tasks.clear();
        tasks.push_back({"1", "Finish PDA OS", 0, 2, "2026-07-15", false, {"dev"}});
        tasks.push_back({"2", "Order PCB rev2", 0, 1, "2026-07-20", false, {"hardware"}});
        tasks.push_back({"3", "Buy groceries", 0, 0, "Today", true, {"life"}});
        
        events.clear();
        events.push_back({"Team Sync", "2026-07-04", "Meeting"});
        events.push_back({"Mom's Birthday", "2026-07-12", "Birthday"});

        notes.clear();
        notes.push_back({"Idea", "Add a battery monitor widget.", th.accent});
        notes.push_back({"WIFI", "SSID: LabNet\nPass: solderiron123", th.bgRaised});

        habits.clear();
        habits.push_back({"Read 10 pages", 14, true});
        habits.push_back({"Code 1 hr", 25, true});

        goals.clear();
        goals.push_back({"Release v1.0", 85});
    }

public:
    void setup(AppContext* ctx, String args) override {
        isTabFocused = true;
        String yr = ctx->getCurrentYear();
        currentYear = yr.toInt();
        if(currentYear < 2000) currentYear = 2026;
        String dStr = ctx->getCurrentDateDDMM(); // "DD.MM"
        if(dStr.length() == 5) {
            selectedDay = dStr.substring(0, 2).toInt();
            currentMonth = dStr.substring(3).toInt();
        } else {
            currentMonth = 7;
            selectedDay = 4;
        }
        
        loadData(ctx);
    }

    void draw(AppContext* ctx) override {
        auto& s = *ctx->extSprite;
        const Theme& th = *ctx->theme;
        s.fillScreen(th.bg);

        // Header
        UI::drawHeader(s, th, ctx->userName.c_str(), ctx->getCurrentTime().c_str());

        // Tab Ribbon
        const char* tabNames[] = {"OVERVIEW", "PLANNER", "TASKS", "NOTES", "GOALS"};
        int tabW = 320 / 5;
        for(int i=0; i<5; i++) {
            bool active = ((int)currentTab == i);
            if(active) {
                s.fillRect(i*tabW, 20, tabW, 20, th.bg);
                s.drawRect(i*tabW, 20, tabW, 20, th.border);
                if (isTabFocused) {
                    s.fillRect(i*tabW + 2, 22, tabW - 4, 2, th.accent);
                }
                s.setTextColor(isTabFocused ? th.accentBright : th.accent);
            } else {
                s.fillRect(i*tabW, 20, tabW, 20, th.bgRaised);
                s.drawRect(i*tabW, 20, tabW, 20, th.border);
                s.setTextColor(th.textDim);
            }
            s.setTextSize(1);
            int tw = strlen(tabNames[i]) * 6;
            s.setCursor(i*tabW + (tabW - tw)/2, 26);
            s.print(tabNames[i]);
            if (active) {
                s.drawLine(i * tabW + 1, 39, i * tabW + tabW - 2, 39, th.bg);
            }
        }

        // Main Content Area
        int contentY = 40;
        int contentH = 184;

        switch (currentTab) {
            case Tab::OVERVIEW: drawOverview(s, th, contentY, contentH, ctx); break;
            case Tab::PLANNER:  drawPlanner(s, th, contentY, contentH, ctx); break;
            case Tab::TASKS:    drawTasks(s, th, contentY, contentH); break;
            case Tab::NOTES:    drawNotes(s, th, contentY, contentH); break;
            case Tab::HABITS:   drawHabits(s, th, contentY, contentH); break;
        }

        if(inputMode != InputMode::NONE) {
            drawInputOverlay(s, th);
        } else if(plannerViewDay) {
            drawPlannerDayView(s, th, ctx);
        }

        // Footer Actions String
        String footerActions = "";
        if (inputMode != InputMode::NONE) {
            footerActions = "[ENTER] Save/Next | [ESC] Cancel";
        } else if (plannerViewDay) {
            footerActions = "[0] New Event | [9] New Task | [ESC] Close";
        } else if (isTabFocused) {
            footerActions = "ARROWS: Nav | ENTER: View Tab";
        } else {
            switch(currentTab) {
                case Tab::TASKS:   footerActions = "[0] New | [6] Done | [7/8] Pri | [9] Del"; break;
                case Tab::NOTES:   footerActions = "[0] New | [6/7/8] Color | [9] Del"; break;
                case Tab::HABITS:  
                    if (selectedGoalIdx == -1) footerActions = "[0] New Goal | [6] Toggle | [9] Del Habit";
                    else footerActions = "[0] New Goal | [=/-] Goal | [9] Del Goal";
                    break;
                case Tab::PLANNER: footerActions = "[6] View Day | [7/8] Prev/Next Month"; break;
                default:           footerActions = "[W] Back to Tabs"; break;
            }
        }

        // Footer
        UI::drawFooter(s, th, footerActions.c_str());
    }

    void drawInputOverlay(M5Canvas& s, const Theme& th) {
        s.fillRect(30, 80, 260, 80, th.bgRaised);
        s.drawRect(30, 80, 260, 80, th.border);
        s.drawRect(31, 81, 258, 78, th.border);

        s.setTextColor(th.accent);
        s.setTextSize(1);
        s.setCursor(40, 90);
        if(inputMode == InputMode::TASK_TITLE) s.print("NEW TASK TITLE:");
        else if(inputMode == InputMode::TASK_DATE) s.print("TASK DATE (YYYY-MM-DD) OR EMPTY:");
        else if(inputMode == InputMode::EVENT_TITLE) s.print("NEW EVENT TITLE:");
        else if(inputMode == InputMode::NOTE_TITLE) s.print("NEW NOTE TITLE:");
        else if(inputMode == InputMode::NOTE_CONTENT) s.print("NOTE CONTENT:");
        else if(inputMode == InputMode::GOAL_TITLE) s.print("NEW GOAL TITLE:");

        s.fillRect(40, 106, 240, 44, th.bgRecessed);
        s.drawRect(40, 106, 240, 44, th.border);
        
        s.setTextColor(th.textPrimary);
        
        int ty = 110;
        if(inputBuffer.length() == 0 && tempDateDefault != "" && inputMode == InputMode::TASK_DATE) {
            s.setTextColor(th.textFaint);
            s.setCursor(44, ty);
            s.print(tempDateDefault.c_str());
            s.setTextColor(th.textPrimary);
        }

        int lineStart = 0;
        for(int j=0; j<inputBuffer.length(); j++) {
            if(inputBuffer[j] == '\n' || j - lineStart > 38) {
                s.setCursor(44, ty);
                s.print(inputBuffer.substring(lineStart, j).c_str());
                ty += 10;
                lineStart = j;
                if(inputBuffer[j] == '\n') lineStart++;
            }
        }
        if(lineStart <= inputBuffer.length()) {
            s.setCursor(44, ty);
            String lastLine = inputBuffer.substring(lineStart);
            s.print(lastLine.c_str());
            
            if((millis() / 500) % 2 == 0) {
                int cx = 44 + lastLine.length() * 6;
                s.fillRect(cx, ty, 2, 8, th.accent);
            }
        }
    }

    void drawPlannerDayView(M5Canvas& s, const Theme& th, AppContext* ctx) {
        s.fillRect(40, 40, 240, 160, th.bgRaised);
        s.drawRect(40, 40, 240, 160, th.border);
        s.drawRect(41, 41, 238, 158, th.border);

        s.setTextColor(th.accent);
        s.setTextSize(1);
        s.setCursor(50, 50);
        s.printf("AGENDA FOR %d-%02d-%02d", currentYear, currentMonth, selectedDay);

        UI::drawSeparator(s, 50, 64, 220, th);

        s.setTextColor(th.textPrimary);
        s.setCursor(50, 72);
        
        char dBuf[16];
        snprintf(dBuf, sizeof(dBuf), "%d-%02d-%02d", currentYear, currentMonth, selectedDay);
        String isoDate = String(dBuf);
        String todayStr = "Today"; 
        
        bool isToday = false;
        String curDate = ctx->getCurrentDateDDMM(); // "DD.MM"
        if(curDate.length() == 5 && curDate.substring(3).toInt() == currentMonth && curDate.substring(0,2).toInt() == selectedDay && ctx->getCurrentYear().toInt() == currentYear) {
            isToday = true;
        }

        int found = 0;
        for(auto& e : events) {
            if(e.date == isoDate && found < 8) {
                s.setTextColor(th.accentBright);
                s.setCursor(50, 72 + found*12);
                s.print("* ");
                s.print(e.title.c_str());
                found++;
            }
        }
        for(auto& t : tasks) {
            if((t.deadline == isoDate || (isToday && t.deadline == "Today")) && found < 8) {
                s.setTextColor(t.completed ? th.textFaint : th.textPrimary);
                s.setCursor(50, 72 + found*12);
                s.print(t.completed ? "[X] " : "[ ] ");
                s.print(t.title.c_str());
                found++;
            }
        }

        if(found == 0) {
            s.setTextColor(th.textFaint);
            s.setCursor(50, 72);
            s.print("No events or tasks scheduled.");
        }

        s.setTextColor(th.textFaint);
        s.setCursor(50, 180);
        s.print("[0] Event   [9] Task   [ESC] Close");
    }

    void drawOverview(M5Canvas& s, const Theme& th, int y, int h, AppContext* ctx) {
        UI::drawRaisedPanel(s, 8, y + 8, 148, 64, th);
        s.setTextColor(th.textFaint);
        s.setCursor(14, y + 14);
        s.print("TIME");
        s.setTextColor(th.accent);
        s.setTextSize(2);
        s.setCursor(14, y + 26);
        s.print(ctx->getCurrentTimeFull().c_str());
        s.setTextSize(1);
        s.setTextColor(th.textDim);
        s.setCursor(14, y + 48);
        s.printf("DATE: %s.%s", ctx->getCurrentDateDDMM().c_str(), ctx->getCurrentYear().c_str());

        UI::drawRaisedPanel(s, 164, y + 8, 148, 64, th);
        s.setTextColor(th.textFaint);
        s.setCursor(170, y + 14);
        s.print("TODAY'S PROGRESS");
        
        int tasksDone = 0;
        for(auto& t : tasks) if(t.completed) tasksDone++;
        s.setTextColor(th.textPrimary);
        s.setCursor(170, y + 30);
        s.printf("Tasks:  %d/%d", tasksDone, tasks.size());

        int habitsDone = 0;
        for(auto& hb : habits) if(hb.doneToday) habitsDone++;
        s.setCursor(170, y + 44);
        s.printf("Habits: %d/%d", habitsDone, habits.size());

        UI::drawRaisedPanel(s, 8, y + 80, 304, 116, th);
        s.setTextColor(th.accentDim);
        s.setCursor(14, y + 88);
        s.print("UPCOMING EVENTS & DEADLINES");
        UI::drawSeparator(s, 14, y + 98, 292, th);

        int drawn = 0;
        for(auto& e : events) {
            if(drawn < 3) {
                s.setTextColor(th.accentBright);
                s.setCursor(14, y + 106 + (drawn * 14));
                s.print(e.date.c_str());
                s.setTextColor(th.textPrimary);
                s.setCursor(84, y + 106 + (drawn * 14));
                s.print(e.title.c_str());
                drawn++;
            }
        }
        for(auto& t : tasks) {
            if(!t.completed && drawn < 6 && t.deadline != "None" && t.deadline != "") {
                s.setTextColor(th.danger);
                s.setCursor(14, y + 106 + (drawn * 14));
                s.print(t.deadline.c_str());
                s.setTextColor(th.textPrimary);
                s.setCursor(84, y + 106 + (drawn * 14));
                s.print(t.title.c_str());
                drawn++;
            }
        }
        if(drawn == 0) {
            s.setTextColor(th.textFaint);
            s.setCursor(14, y + 106);
            s.print("No upcoming events or tasks.");
        }
    }

    void drawPlanner(M5Canvas& s, const Theme& th, int y, int h, AppContext* ctx) {
        UI::drawRaisedPanel(s, 8, y + 8, 304, 188, th);
        
        const char* monthNames[] = {"", "JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE", "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"};
        
        s.setTextColor(th.accent);
        s.setTextSize(1);
        s.setCursor(110, y + 16);
        s.printf("%s %d", monthNames[currentMonth], currentYear);

        const char* days[] = {"MO", "TU", "WE", "TH", "FR", "SA", "SU"};
        for(int i = 0; i < 7; i++) {
            s.setTextColor(th.textDim);
            s.setCursor(20 + i * 40, y + 32);
            s.print(days[i]);
        }
        UI::drawSeparator(s, 14, y + 42, 292, th);

        int startDow = dayOfWeek(currentYear, currentMonth, 1);
        int numDays = daysInMonth(currentMonth, currentYear);
        int day = 1;
        
        for(int row = 0; row < 6; row++) {
            for(int col = 0; col < 7; col++) {
                if(row == 0 && col < startDow) continue;
                if(day > numDays) break;

                int dx = 20 + col * 40;
                int dy = y + 52 + row * 26;
                
                if(day == selectedDay) {
                    s.fillRoundRect(dx-4, dy-2, 20, 14, 3, th.accent);
                    s.setTextColor(th.bg);
                } else {
                    s.setTextColor(th.textPrimary);
                }
                
                s.setCursor(dx, dy);
                s.printf("%d", day);
                day++;
            }
        }

        s.setTextColor(th.textFaint);
        s.setCursor(14, y + 178);
        s.printf("Selected: %d %s - View Day for details", selectedDay, monthNames[currentMonth]);
    }

    void drawTasks(M5Canvas& s, const Theme& th, int y, int h) {
        int itemsPerPage = 10;
        int rowH = 20;

        for (int i = 0; i < itemsPerPage; i++) {
            int idx = taskScroll + i;
            if (idx >= (int)tasks.size()) break;

            int rowY = y + 4 + i * rowH;

            if (idx == selectedTaskIdx) {
                s.fillRect(8, rowY, 304, rowH - 2, th.selectBg);
                s.drawRect(8, rowY, 304, rowH - 2, th.accent);
                s.fillRect(8, rowY + 2, 2, rowH - 6, th.accent);
            }

            auto& t = tasks[idx];
            int indent = t.level * 12;
            
            s.drawRect(16 + indent, rowY + 3, 10, 10, th.border);
            if (t.completed) {
                s.drawLine(16 + indent, rowY + 3, 26 + indent, rowY + 13, th.indicator);
                s.drawLine(26 + indent, rowY + 3, 16 + indent, rowY + 13, th.indicator);
            }

            uint16_t pCol = th.textFaint;
            if(t.priority == 1) pCol = th.accentDim;
            if(t.priority == 2) pCol = th.danger;
            s.setTextColor(pCol);
            s.setCursor(32 + indent, rowY + 4);
            s.print(t.priority == 2 ? "!!!" : (t.priority == 1 ? "!!" : "!"));

            s.setTextColor(t.completed ? th.textFaint : th.textPrimary);
            s.setCursor(54 + indent, rowY + 4);
            s.print(t.title.c_str());

            s.setTextColor(th.textDim);
            s.setCursor(240, rowY + 4);
            s.print(t.deadline.c_str());
        }

        if (tasks.size() > itemsPerPage) {
            int sbH = itemsPerPage * rowH;
            int thumbH = max(8, sbH * itemsPerPage / (int)tasks.size());
            int thumbY = y + 4 + (sbH - thumbH) * taskScroll / max(1, (int)tasks.size() - itemsPerPage);
            s.drawRect(314, y + 4, 4, sbH, th.border);
            s.fillRect(315, thumbY, 2, thumbH, th.accent);
        }
    }

    void drawNotes(M5Canvas& s, const Theme& th, int y, int h) {
        int notesPerPage = 4;
        int page = noteScroll / notesPerPage;
        
        for(int i=0; i<4; i++) {
            int idx = page * 4 + i;
            if(idx >= (int)notes.size()) break;

            int col = i % 2;
            int row = i / 2;
            
            int nx = 8 + col * 154;
            int ny = y + 8 + row * 98;
            int nw = 148;
            int nh = 90;

            auto& n = notes[idx];
            
            s.fillRect(nx, ny, nw, nh, n.color);
            s.drawRect(nx, ny, nw, nh, th.border);
            
            if (idx == selectedNoteIdx) {
                s.drawRect(nx+1, ny+1, nw-2, nh-2, th.accentBright);
                s.drawRect(nx+2, ny+2, nw-4, nh-4, th.accentBright);
            }

            uint16_t textCol = (n.color == th.accent || n.color == th.danger || n.color == th.indicator) ? th.bg : th.textPrimary;
            uint16_t lineCol = (n.color == th.accent || n.color == th.danger || n.color == th.indicator) ? th.bgPanel : th.border;

            s.setTextColor(textCol);
            s.setTextSize(1);
            s.setCursor(nx + 6, ny + 6);
            s.print(n.title.c_str());
            
            s.drawLine(nx + 4, ny + 16, nx + nw - 4, ny + 16, lineCol);
            
            String c = n.content;
            if(c.length() > 80) c = c.substring(0, 77) + "...";
            
            int ty = ny + 20;
            int lineStart = 0;
            for(int j=0; j<c.length(); j++) {
                if(c[j] == '\n' || j - lineStart > 22) {
                    s.setCursor(nx + 6, ty);
                    s.print(c.substring(lineStart, j).c_str());
                    ty += 10;
                    lineStart = j;
                    if(c[j] == '\n') lineStart++;
                }
            }
            if(lineStart < c.length()) {
                s.setCursor(nx + 6, ty);
                s.print(c.substring(lineStart).c_str());
            }
        }
    }

    void drawHabits(M5Canvas& s, const Theme& th, int y, int h) {
        UI::drawRaisedPanel(s, 8, y + 8, 150, 188, th);
        s.setTextColor(th.accentDim);
        s.setCursor(14, y + 14);
        s.print("HABIT TRACKER");
        UI::drawSeparator(s, 14, y + 24, 138, th);

        for(int i=0; i<habits.size(); i++) {
            auto& hb = habits[i];
            int hy = y + 32 + i * 26;
            
            if(i == selectedHabitIdx && selectedGoalIdx == -1) {
                s.fillRect(10, hy, 146, 24, th.selectBg);
            }

            s.drawRect(14, hy + 6, 12, 12, th.border);
            if(hb.doneToday) {
                s.fillRect(16, hy + 8, 8, 8, th.indicator);
            }
            s.setTextColor(th.textPrimary);
            s.setCursor(32, hy + 8);
            s.print(hb.name.c_str());
            
            s.setTextColor(th.textFaint);
            s.setCursor(120, hy + 8);
            s.printf("%d\x18", hb.streak);
        }

        UI::drawRaisedPanel(s, 164, y + 8, 148, 188, th);
        s.setTextColor(th.accentDim);
        s.setCursor(170, y + 14);
        s.print("GOALS");
        UI::drawSeparator(s, 170, y + 24, 136, th);

        for(int i=0; i<goals.size(); i++) {
            auto& g = goals[i];
            int gy = y + 32 + i * 40;
            
            if(i == selectedGoalIdx) {
                s.fillRect(166, gy-4, 144, 32, th.selectBg);
            }

            s.setTextColor(th.textPrimary);
            s.setCursor(170, gy);
            s.print(g.title.c_str());
            
            s.drawRect(170, gy + 14, 136, 8, th.border);
            s.fillRect(171, gy + 15, (134 * g.progressPct) / 100, 6, th.accent);
            
            s.setTextColor(th.textFaint);
            s.setCursor(280, gy);
            s.printf("%d%%", g.progressPct);
        }
    }

    void drawStatus(AppContext* ctx) override {
        auto& is = *ctx->intDisplay;
        const Theme& th = *ctx->theme;
        
        is.setTextSize(1);
        
        // Date Badge
        is.setTextColor(th.accentBright);
        is.setTextSize(2);
        
        const char* daysOfWeek[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
        
        int tMonth = currentMonth;
        int tDay = selectedDay;
        String curDate = ctx->getCurrentDateDDMM(); // "DD.MM"
        if(curDate.length() == 5) {
            tDay = curDate.substring(0, 2).toInt();
            tMonth = curDate.substring(3).toInt();
        }
        int yr = ctx->getCurrentYear().toInt();
        if(yr < 2000) yr = 2026;
        int tdow = dayOfWeek(yr, tMonth, tDay);
        
        char dBuf[32];
        const char* mNames[] = {"", "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
        snprintf(dBuf, sizeof(dBuf), "%s / %02d %s", daysOfWeek[tdow], tDay, mNames[tMonth]);
        
        int txtW = strlen(dBuf) * 12;
        is.setCursor((240 - txtW) / 2, 20);
        is.print(dBuf);
        
        // Task Progress Bar
        int tasksDone = 0;
        for(auto& t : tasks) if(t.completed) tasksDone++;
        int tasksTotal = tasks.size();
        
        is.setTextSize(1);
        is.setTextColor(th.textPrimary);
        char pBuf[32];
        snprintf(pBuf, sizeof(pBuf), "Tasks Completed: %d / %d", tasksDone, tasksTotal);
        int pTxtW = strlen(pBuf) * 6;
        is.setCursor((240 - pTxtW) / 2, 55);
        is.print(pBuf);
        
        is.drawRect(20, 70, 200, 16, th.border);
        is.drawRect(21, 71, 198, 14, th.border);
        if(tasksTotal > 0) {
            int fillW = (194 * tasksDone) / tasksTotal;
            is.fillRect(23, 73, fillW, 10, th.indicator);
        }
        
        // Next Event / Goal Indicator
        is.setTextColor(th.accentDim);
        is.setCursor(20, 100);
        
        if (currentTab == Tab::HABITS && selectedHabitIdx >= 0 && selectedHabitIdx < habits.size() && selectedGoalIdx == -1) {
            is.printf("Habit: %s (%d Days \x18)", habits[selectedHabitIdx].name.c_str(), habits[selectedHabitIdx].streak);
        } else if (currentTab == Tab::HABITS && selectedGoalIdx >= 0 && selectedGoalIdx < goals.size()) {
            is.printf("Goal: %s (%d%%)", goals[selectedGoalIdx].title.c_str(), goals[selectedGoalIdx].progressPct);
        } else {
            String nextEventTitle = "None";
            char todayIso[16];
            snprintf(todayIso, sizeof(todayIso), "%d-%02d-%02d", yr, tMonth, tDay);
            String tIso = String(todayIso);
            
            for(auto& e : events) {
                if(e.date >= tIso) {
                    nextEventTitle = e.date + " " + e.title;
                    break;
                }
            }
            if(nextEventTitle.length() > 30) nextEventTitle = nextEventTitle.substring(0, 27) + "...";
            is.print("Next: ");
            is.setTextColor(th.textPrimary);
            is.print(nextEventTitle.c_str());
        }
    }

    bool handleInput(AppContext* ctx, char key, bool isDel, bool isEnter) override {
        if(inputMode != InputMode::NONE) {
            if(key == 27 || key == '`') { 
                inputMode = InputMode::NONE;
                inputBuffer = "";
                return true;
            }
            if(isDel && inputBuffer.length() > 0) {
                inputBuffer.remove(inputBuffer.length() - 1);
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
            } else if (isEnter) {
                if(inputMode == InputMode::TASK_DATE && inputBuffer.length() == 0 && tempDateDefault != "") {
                    inputBuffer = tempDateDefault;
                }
                
                if(inputBuffer.length() > 0 || inputMode == InputMode::TASK_DATE) {
                    if(inputMode == InputMode::TASK_TITLE) {
                        tempTaskTitle = inputBuffer;
                        inputBuffer = "";
                        inputMode = InputMode::TASK_DATE; 
                        return true;
                    } else if(inputMode == InputMode::TASK_DATE) {
                        Task t; t.title = tempTaskTitle; t.level = 0; t.priority = 1; 
                        t.deadline = (inputBuffer.length() > 0) ? inputBuffer : "None"; 
                        t.completed = false;
                        tasks.push_back(t);
                        saveData(ctx);
                        inputMode = InputMode::NONE;
                    } else if(inputMode == InputMode::NOTE_TITLE) {
                        Note n; n.title = inputBuffer; n.color = ctx->theme->accent;
                        notes.push_back(n);
                        inputBuffer = "";
                        inputMode = InputMode::NOTE_CONTENT;
                        return true;
                    } else if(inputMode == InputMode::NOTE_CONTENT) {
                        if(!notes.empty()) {
                            notes.back().content = inputBuffer;
                            saveData(ctx);
                        }
                        inputMode = InputMode::NONE;
                    } else if(inputMode == InputMode::GOAL_TITLE) {
                        Goal g; g.title = inputBuffer; g.progressPct = 0;
                        goals.push_back(g);
                        saveData(ctx);
                        inputMode = InputMode::NONE;
                    } else if(inputMode == InputMode::EVENT_TITLE) {
                        Event e; e.title = inputBuffer; e.date = tempDateDefault; e.type = "Event";
                        events.push_back(e);
                        saveData(ctx);
                        inputMode = InputMode::NONE;
                    }
                } else {
                    inputMode = InputMode::NONE; 
                }
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CONFIRM);
                inputBuffer = "";
            } else if (key >= 32 && key <= 126) {
                inputBuffer += String(key);
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
            }
            return true;
        }

        if(plannerViewDay) {
            if(key == 27 || key == '`') {
                plannerViewDay = false;
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
            } else if(key == '0') {
                char dBuf[16];
                snprintf(dBuf, sizeof(dBuf), "%d-%02d-%02d", currentYear, currentMonth, selectedDay);
                tempDateDefault = String(dBuf);
                inputMode = InputMode::EVENT_TITLE;
                inputBuffer = "";
            } else if(key == '9') {
                char dBuf[16];
                snprintf(dBuf, sizeof(dBuf), "%d-%02d-%02d", currentYear, currentMonth, selectedDay);
                tempDateDefault = String(dBuf);
                inputMode = InputMode::TASK_TITLE;
                inputBuffer = "";
            }
            return true;
        }

        if(key >= '1' && key <= '5') {
            currentTab = (Tab)(key - '1');
            isTabFocused = true;
            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
            return false;
        }

        if (isTabFocused) {
            if (key == 'a' || key == ',' || key == 'd' || key == '/') {
                int dir = (key == 'a' || key == ',') ? -1 : 1;
                int nTab = (int)currentTab + dir;
                if (nTab < 0) nTab = 4;
                if (nTab > 4) nTab = 0;
                currentTab = (Tab)nTab;
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                return false;
            } else if (key == 's' || key == '.') {
                isTabFocused = false;
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                return false;
            } else if (key == 'w' || key == ';') {
                return false;
            } else if (isEnter) {
                isTabFocused = false;
                SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
                return false;
            }
        }

        switch (currentTab) {
            case Tab::OVERVIEW: {
                if (key == 'w' || key == ';') {
                    isTabFocused = true;
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                }
                break;
            }
            case Tab::PLANNER: {
                if (key == 'd' || key == '/') {
                    selectedDay++;
                    if(selectedDay > daysInMonth(currentMonth, currentYear)) {
                        selectedDay = 1;
                        if(currentMonth == 12) { currentMonth = 1; currentYear++; } else { currentMonth++; }
                    }
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                } else if (key == 'a' || key == ',') {
                    selectedDay--;
                    if(selectedDay < 1) {
                        if(currentMonth == 1) { currentMonth = 12; currentYear--; } else { currentMonth--; }
                        selectedDay = daysInMonth(currentMonth, currentYear);
                    }
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                } else if (key == 's' || key == '.') {
                    selectedDay += 7;
                    if(selectedDay > daysInMonth(currentMonth, currentYear)) {
                        selectedDay -= daysInMonth(currentMonth, currentYear);
                        if(currentMonth == 12) { currentMonth = 1; currentYear++; } else { currentMonth++; }
                    }
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                } else if (key == 'w' || key == ';') {
                    if (selectedDay <= 7) {
                        isTabFocused = true;
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                    } else {
                        selectedDay -= 7;
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                    }
                } else if (key == '6' || isEnter) {
                    plannerViewDay = true;
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
                } else if (key == '7') { 
                    if(currentMonth == 1) { currentMonth = 12; currentYear--; } else { currentMonth--; }
                    if(selectedDay > daysInMonth(currentMonth, currentYear)) selectedDay = daysInMonth(currentMonth, currentYear);
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                } else if (key == '8') { 
                    if(currentMonth == 12) { currentMonth = 1; currentYear++; } else { currentMonth++; }
                    if(selectedDay > daysInMonth(currentMonth, currentYear)) selectedDay = daysInMonth(currentMonth, currentYear);
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                }
                break;
            }
            case Tab::TASKS: {
                if (key == 's' || key == '.') {
                    if (selectedTaskIdx < (int)tasks.size() - 1) {
                        selectedTaskIdx++;
                        if (selectedTaskIdx >= taskScroll + 10) taskScroll++;
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                    }
                } else if (key == 'w' || key == ';') {
                    if (selectedTaskIdx == 0 || tasks.empty()) {
                        isTabFocused = true;
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                    } else if (selectedTaskIdx > 0) {
                        selectedTaskIdx--;
                        if (selectedTaskIdx < taskScroll) taskScroll--;
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                    }
                } else if (key == '0') {
                    inputMode = InputMode::TASK_TITLE;
                    inputBuffer = "";
                    tempDateDefault = "";
                } else if (key == '6' || isEnter) {
                    if(!tasks.empty()) {
                        tasks[selectedTaskIdx].completed = !tasks[selectedTaskIdx].completed;
                        saveData(ctx);
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
                    }
                } else if (key == '7') {
                    if(!tasks.empty() && tasks[selectedTaskIdx].priority < 2) {
                        tasks[selectedTaskIdx].priority++;
                        saveData(ctx);
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
                    }
                } else if (key == '8') {
                    if(!tasks.empty() && tasks[selectedTaskIdx].priority > 0) {
                        tasks[selectedTaskIdx].priority--;
                        saveData(ctx);
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
                    }
                } else if (key == '9') {
                    if(!tasks.empty()) {
                        tasks.erase(tasks.begin() + selectedTaskIdx);
                        if(selectedTaskIdx >= tasks.size() && selectedTaskIdx > 0) selectedTaskIdx--;
                        saveData(ctx);
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::ERROR);
                    }
                }
                break;
            }
            case Tab::NOTES: {
                if (key == 'd' || key == '/') {
                    if (selectedNoteIdx < (int)notes.size() - 1) {
                        selectedNoteIdx++;
                        if (selectedNoteIdx >= noteScroll + 4) noteScroll+=4;
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                    }
                } else if (key == 'a' || key == ',') {
                    if (selectedNoteIdx > 0) {
                        selectedNoteIdx--;
                        if (selectedNoteIdx < noteScroll) noteScroll-=4;
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                    }
                } else if (key == 's' || key == '.') {
                    if (selectedNoteIdx + 2 < (int)notes.size()) {
                        selectedNoteIdx += 2;
                        if (selectedNoteIdx >= noteScroll + 4) noteScroll+=4;
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                    }
                } else if (key == 'w' || key == ';') {
                    if (selectedNoteIdx <= 1 || notes.empty()) {
                        isTabFocused = true;
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                    } else if (selectedNoteIdx - 2 >= 0) {
                        selectedNoteIdx -= 2;
                        if (selectedNoteIdx < noteScroll) noteScroll-=4;
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                    }
                } else if (key == '0') {
                    inputMode = InputMode::NOTE_TITLE;
                    inputBuffer = "";
                } else if (key == '6' && !notes.empty()) {
                    notes[selectedNoteIdx].color = ctx->theme->accent;
                    saveData(ctx);
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
                } else if (key == '7' && !notes.empty()) {
                    notes[selectedNoteIdx].color = ctx->theme->danger;
                    saveData(ctx);
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
                } else if (key == '8' && !notes.empty()) {
                    notes[selectedNoteIdx].color = ctx->theme->bgRaised;
                    saveData(ctx);
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
                } else if (key == '9' && !notes.empty()) {
                    notes.erase(notes.begin() + selectedNoteIdx);
                    if(selectedNoteIdx >= notes.size() && selectedNoteIdx > 0) selectedNoteIdx--;
                    saveData(ctx);
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::ERROR);
                }
                break;
            }
            case Tab::HABITS: {
                 if (key == 's' || key == '.') {
                    if(selectedGoalIdx == -1) {
                        if (selectedHabitIdx < (int)habits.size() - 1) {
                            selectedHabitIdx++;
                            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                        }
                    } else {
                        if (selectedGoalIdx < (int)goals.size() - 1) {
                            selectedGoalIdx++;
                            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                        }
                    }
                } else if (key == 'w' || key == ';') {
                    if(selectedGoalIdx != -1) {
                        if (selectedGoalIdx > 0) {
                            selectedGoalIdx--;
                            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                        } else {
                            selectedGoalIdx = -1;
                            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                        }
                    } else {
                        if (selectedHabitIdx > 0) {
                            selectedHabitIdx--;
                            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                        } else if (selectedHabitIdx == 0 || habits.empty()) {
                            isTabFocused = true;
                            SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                        }
                    }
                } else if (key == 'd' || key == '/') {
                    if(selectedGoalIdx == -1 && goals.size() > 0) {
                        selectedGoalIdx = 0;
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                    }
                } else if (key == 'a' || key == ',') {
                    if(selectedGoalIdx != -1 && habits.size() > 0) {
                        selectedGoalIdx = -1;
                        if(selectedHabitIdx >= habits.size()) selectedHabitIdx = habits.size() - 1;
                        if(selectedHabitIdx < 0) selectedHabitIdx = 0;
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::NAVIGATE);
                    }
                } else if (key == '0') {
                    inputMode = InputMode::GOAL_TITLE;
                    inputBuffer = "";
                } else if (key == '6' || isEnter) {
                    if(selectedGoalIdx == -1 && !habits.empty()) { 
                        habits[selectedHabitIdx].doneToday = !habits[selectedHabitIdx].doneToday;
                        if(habits[selectedHabitIdx].doneToday) habits[selectedHabitIdx].streak++;
                        else habits[selectedHabitIdx].streak--;
                        saveData(ctx);
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
                    }
                } else if (key == '=' && selectedGoalIdx != -1 && !goals.empty()) { 
                    if(goals[selectedGoalIdx].progressPct < 100) goals[selectedGoalIdx].progressPct += 5;
                    if(goals[selectedGoalIdx].progressPct > 100) goals[selectedGoalIdx].progressPct = 100;
                    saveData(ctx);
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
                } else if ((key == '-' || key == '_') && selectedGoalIdx != -1 && !goals.empty()) { 
                    if(goals[selectedGoalIdx].progressPct > 0) goals[selectedGoalIdx].progressPct -= 5;
                    if(goals[selectedGoalIdx].progressPct < 0) goals[selectedGoalIdx].progressPct = 0;
                    saveData(ctx);
                    SysAudio.play(ctx->theme->interactionStyle, SoundEvent::CLICK);
                } else if (key == '9') {
                    if(selectedGoalIdx == -1 && !habits.empty()) { 
                        habits.erase(habits.begin() + selectedHabitIdx);
                        if(selectedHabitIdx >= habits.size() && selectedHabitIdx > 0) selectedHabitIdx--;
                        saveData(ctx);
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::ERROR);
                    } else if(selectedGoalIdx != -1 && !goals.empty()) { 
                        goals.erase(goals.begin() + selectedGoalIdx);
                        if(selectedGoalIdx >= goals.size() && selectedGoalIdx > 0) selectedGoalIdx--;
                        saveData(ctx);
                        SysAudio.play(ctx->theme->interactionStyle, SoundEvent::ERROR);
                    }
                }
                break;
            }
            default:
                break;
        }

        return false;
    }
};
