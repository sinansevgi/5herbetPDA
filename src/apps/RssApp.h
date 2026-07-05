#pragma once
#include "../App.h"
#include <SD.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <vector>

class RssApp : public App {
private:
    struct RssFeed {
        String name;
        String url;
    };

    enum class RssMode {
        FEED_LIST,
        FETCHING,
        READING
    };

    RssMode currentMode = RssMode::FEED_LIST;
    std::vector<RssFeed> feeds;
    int feedMenuIndex = 0;
    int titleMenuIndex = 0;
    int scrollOffset = 0;
    
    std::vector<String> currentTitles;
    String fetchError = "";

    void loadFeeds(AppContext* ctx) {
        feeds.clear();
        if (!ctx->sdAvailable) return;
        
        String path = "/5herbetPDA/rss_feeds_v3.json";
        if (!SD.exists(path)) {
            // Create default
            File f = SD.open(path, FILE_WRITE);
            if (f) {
                f.println("{\"feeds\": [");
                f.println("  {\"name\": \"OMG Ubuntu\", \"url\": \"https://www.omgubuntu.co.uk/feed\"},");
                f.println("  {\"name\": \"BleepingComputer\", \"url\": \"https://www.bleepingcomputer.com/feed/\"}");
                f.println("]}");
                f.close();
            }
        }

        File f = SD.open(path, FILE_READ);
        if (f) {
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, f);
            if (!err) {
                JsonArray arr = doc["feeds"];
                for (JsonObject item : arr) {
                    RssFeed feed;
                    feed.name = item["name"].as<String>();
                    feed.url = item["url"].as<String>();
                    feeds.push_back(feed);
                }
            }
            f.close();
        }
        
        if (feeds.empty()) {
            feeds.push_back({"No Feeds Configured", ""});
        }
    }

    String getDomain(String url) {
        if (url.startsWith("http://")) url = url.substring(7);
        else if (url.startsWith("https://")) url = url.substring(8);
        int slash = url.indexOf('/');
        if (slash != -1) url = url.substring(0, slash);
        if (url.startsWith("www.")) url = url.substring(4);
        return url;
    }

    void fetchFeed(AppContext* ctx, String url) {
        currentTitles.clear();
        fetchError = "";
        
        unsigned long now = ctx->epochBase + (millis() - ctx->epochMillis) / 1000UL;
        String cachePath = "/5herbetPDA/rss_cache_" + String(feedMenuIndex) + ".txt";
        bool hasCache = ctx->sdAvailable && SD.exists(cachePath);
        
        auto loadFromCache = [&]() -> bool {
            currentTitles.clear();
            File f = SD.open(cachePath, FILE_READ);
            if (f) {
                f.readStringUntil('\n'); // skip timestamp
                while (f.available()) {
                    String t = f.readStringUntil('\n');
                    t.trim();
                    if (t.length() > 0) currentTitles.push_back(t);
                }
                f.close();
            }
            if (!currentTitles.empty()) {
                titleMenuIndex = 0;
                scrollOffset = 0;
                currentMode = RssMode::READING;
                return true;
            }
            return false;
        };

        // 1. Try valid cache first (under 3 hours)
        if (hasCache) {
            File f = SD.open(cachePath, FILE_READ);
            if (f) {
                String timeStr = f.readStringUntil('\n');
                unsigned long cachedTime = timeStr.toInt();
                f.close();
                if (now >= cachedTime && (now - cachedTime) < 10800) {
                    if (loadFromCache()) {
                        ctx->showNotification("Loaded from SD cache");
                        return;
                    }
                }
            }
        }

        // 2. Check WiFi connection
        if (WiFi.status() != WL_CONNECTED) {
            if (hasCache && loadFromCache()) {
                ctx->showNotification("Offline - loaded cache");
                return;
            }
            fetchError = "No WiFi & no cache";
            ctx->showNotification("Offline - no cache");
            currentMode = RssMode::FEED_LIST;
            return;
        }

        if (url == "") {
            fetchError = "Invalid URL";
            currentMode = RssMode::FEED_LIST;
            return;
        }

        HTTPClient http;
        bool isHttps = url.startsWith("https");
        WiFiClientSecure *secureClient = nullptr;
        
        if (isHttps) {
            secureClient = new WiFiClientSecure();
            secureClient->setInsecure(); // Skip certificate validation
            http.begin(*secureClient, url);
        } else {
            http.begin(url);
        }

        http.setUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) 5herbetPDA RSS/1.0");
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

        int httpCode = http.GET();
        if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                WiFiClient *stream = http.getStreamPtr();
                
                String window = "";
                String titleContent = "";
                bool inItem = false;
                bool inTitle = false;
                
                int maxTitles = 15;
                int timeout = 5000;
                unsigned long start = millis();
                unsigned long lastAnimTime = 0;
                
                while (http.connected() && (stream->available() || stream->connected()) && currentTitles.size() < maxTitles) {
                    if (millis() - start > timeout) {
                        fetchError = "Stream timeout";
                        break;
                    }
                    
                    if (stream->available()) {
                        char c = stream->read();
                        start = millis(); // reset timeout
                        
                        window += c;
                        if (window.length() > 16) window.remove(0, 1);
                        
                        if (!inItem) {
                            if (window.endsWith("<item>")) {
                                inItem = true;
                            }
                        } else if (!inTitle) {
                            if (window.endsWith("<title>")) {
                                inTitle = true;
                                titleContent = "";
                            } else if (window.endsWith("</item>")) {
                                inItem = false;
                            }
                        } else {
                            titleContent += c;
                            if (window.endsWith("</title>")) {
                                inTitle = false;
                                if (titleContent.length() >= 8) {
                                    titleContent = titleContent.substring(0, titleContent.length() - 8);
                                }
                                
                                titleContent.replace("<![CDATA[", "");
                                titleContent.replace("]]>", "");
                                
                                titleContent.replace("&amp;", "&");
                                titleContent.replace("&lt;", "<");
                                titleContent.replace("&gt;", ">");
                                titleContent.replace("&quot;", "\"");
                                titleContent.replace("&#39;", "'");
                                titleContent.trim();
                                
                                if (titleContent.length() > 0) {
                                    currentTitles.push_back(titleContent);
                                }
                            } else if (titleContent.length() > 256) {
                                inTitle = false;
                            }
                        }
                    } else {
                        delay(1);
                    }

                    // Periodically update the displays to animate the percentage ticker and marquee
                    if (millis() - lastAnimTime > 150) {
                        lastAnimTime = millis();
                        
                        ctx->intDisplay->fillScreen(ctx->theme->bg);
                        drawStatus(ctx);
                        ((M5Canvas*)ctx->intDisplay)->pushSprite(0, 0);
                        
                        ctx->extSprite->fillScreen(ctx->theme->bg);
                        draw(ctx);
                        ctx->extSprite->pushSprite(0, 0);
                        
                        SysAudio.update();
                    }
                }
            } else {
                fetchError = "HTTP Code: " + String(httpCode);
            }
        } else {
            fetchError = "HTTP Failed: " + http.errorToString(httpCode);
        }

        http.end();
        if (secureClient) {
            delete secureClient;
        }

        if (currentTitles.empty()) {
            if (fetchError == "") fetchError = "No items found.";
            if (hasCache && loadFromCache()) {
                ctx->showNotification("Fetch failed - loaded cache");
                fetchError = "";
            } else {
                ctx->showNotification("Fetch failed");
            }
        } else {
            if (ctx->sdAvailable) {
                File f = SD.open(cachePath, FILE_WRITE);
                if (f) {
                    f.println(now);
                    for (String t : currentTitles) {
                        f.println(t);
                    }
                    f.close();
                }
            }
            ctx->showNotification("Fetched fresh feed");
        }

        titleMenuIndex = 0;
        scrollOffset = 0;
        currentMode = currentTitles.empty() ? RssMode::FEED_LIST : RssMode::READING;
    }

    void drawWrappedText(M5Canvas& s, String text, int x, int y, int w, int maxLines) {
        int line = 0;
        int cx = x;
        int cy = y;
        s.setCursor(cx, cy);
        
        int spaceWidth = s.textWidth(" ");
        String currentWord = "";
        
        auto drawWord = [&](String word) {
            int ww = s.textWidth(word);
            if (cx + ww > x + w) {
                line++;
                cy += s.fontHeight() + 4; // Add line spacing
                cx = x;
            }
            if (line < maxLines) {
                s.setCursor(cx, cy);
                s.print(word);
                cx += ww;
            }
        };
        
        for (int i=0; i<text.length(); i++) {
            if (line >= maxLines) break;
            char c = text[i];
            if (c == ' ' || c == '\n') {
                if (currentWord.length() > 0) {
                    drawWord(currentWord);
                    currentWord = "";
                }
                if (c == ' ') {
                    if (cx + spaceWidth <= x + w && cx > x) cx += spaceWidth;
                } else if (c == '\n') {
                    line++;
                    cy += s.fontHeight() + 4;
                    cx = x;
                }
            } else {
                currentWord += c;
            }
        }
        if (currentWord.length() > 0 && line < maxLines) {
            drawWord(currentWord);
        }
    }

public:
    void setup(AppContext* ctx, String args) override {
        currentMode = RssMode::FEED_LIST;
        feedMenuIndex = 0;
        titleMenuIndex = 0;
        scrollOffset = 0;
        loadFeeds(ctx);
    }

    void draw(AppContext* ctx) override {
        auto& s = *ctx->extSprite;
        const Theme& th = *ctx->theme;

        s.fillScreen(th.bg);
        { char tb[8]; ctx->getTime(tb, sizeof(tb)); UI::drawHeader(s, th, ctx->userName.c_str(), tb); }

        if (currentMode == RssMode::FEED_LIST) {
            // Render a grid of feed source cards on the external screen (2 columns, scrollable)
            int startRow = scrollOffset;
            
            for (int r = 0; r < 2; r++) {
                int rowIdx = startRow + r;
                int rowY = 30 + r * 90; // y = 30 or 120
                
                for (int col = 0; col < 2; col++) {
                    int feedIdx = rowIdx * 2 + col;
                    if (feedIdx >= feeds.size()) break;
                    
                    int colX = (col == 0) ? 15 : 165;
                    bool isSelected = (feedIdx == feedMenuIndex);
                    
                    // Card background & borders
                    if (isSelected) {
                        s.fillRect(colX, rowY, 140, 80, th.selectBg);
                        s.drawRect(colX, rowY, 140, 80, th.accentBright);
                        s.drawRect(colX - 1, rowY - 1, 142, 82, th.accent);
                    } else {
                        s.fillRect(colX, rowY, 140, 80, th.bgRecessed);
                        s.drawRect(colX, rowY, 140, 80, th.border);
                    }
                    
                    uint16_t primaryCol = isSelected ? th.accentBright : th.textPrimary;
                    uint16_t dimCol = isSelected ? th.accent : th.textDim;
                    uint16_t faintCol = isSelected ? th.accentDim : th.textFaint;
                    
                    // Draw Stylized retro RSS transmitter icon inside the card
                    int cx = colX + 22;
                    int cy = rowY + 40;
                    
                    // Tower base
                    s.drawLine(cx, cy - 10, cx - 8, cy + 18, dimCol);
                    s.drawLine(cx, cy - 10, cx + 8, cy + 18, dimCol);
                    s.drawLine(cx - 5, cy + 2, cx + 5, cy + 2, dimCol);
                    s.drawLine(cx - 7, cy + 10, cx + 7, cy + 10, dimCol);
                    s.fillCircle(cx, cy - 10, 2, primaryCol);
                    
                    // Radio waves (arcs)
                    s.drawArc(cx, cy - 10, 6, 7, 135, 225, primaryCol);
                    s.drawArc(cx, cy - 10, 11, 12, 135, 225, dimCol);
                    s.drawArc(cx, cy - 10, 6, 7, 315, 405, primaryCol);
                    s.drawArc(cx, cy - 10, 11, 12, 315, 405, dimCol);
                    
                    // Draw title wrapped to 3 lines (x = colX + 44, width = 90)
                    s.setTextSize(1);
                    s.setTextColor(primaryCol);
                    drawWrappedText(s, feeds[feedIdx].name, colX + 44, rowY + 10, 90, 3);
                    
                    // Draw domain at bottom
                    s.setTextColor(faintCol);
                    String domain = getDomain(feeds[feedIdx].url);
                    if (domain.length() > 15) domain = domain.substring(0, 12) + "...";
                    s.setCursor(colX + 44, rowY + 62);
                    s.print(domain);
                }
            }
            
            if (fetchError != "") {
                UI::drawFooter(s, th, ("Err: " + fetchError).c_str());
            } else {
                UI::drawFooter(s, th, "W/S/A/D to navigate, Enter to load");
            }
        } 
        else if (currentMode == RssMode::FETCHING) {
            s.setTextColor(th.accent);
            s.setTextSize(2);
            s.setCursor(10, 100);
            s.print("Fetching RSS...");
            
            UI::drawLoading(s, th, millis() / 50);
            UI::drawFooter(s, th, "Downloading...");
        }
        else if (currentMode == RssMode::READING) {
            // Master-Detail split pane
            int maxTitlesDisp = 6;
            if (titleMenuIndex < scrollOffset) {
                scrollOffset = titleMenuIndex;
            } else if (titleMenuIndex >= scrollOffset + maxTitlesDisp) {
                scrollOffset = titleMenuIndex - (maxTitlesDisp - 1);
            }
            
            // Left Pane (45% width - 144px): Display the scrollable list of article titles
            for (int i = 0; i < maxTitlesDisp; i++) {
                int titleIdx = scrollOffset + i;
                if (titleIdx >= currentTitles.size()) break;
                
                int itemY = 25 + i * 32;
                bool isSelected = (titleIdx == titleMenuIndex);
                
                if (isSelected) {
                    s.fillRect(5, itemY, 144, 30, th.selectBg);
                    s.drawRect(5, itemY, 144, 30, th.accentBright);
                    s.setTextColor(th.accentBright);
                } else {
                    s.fillRect(5, itemY, 144, 30, th.bgRecessed);
                    s.drawRect(5, itemY, 144, 30, th.border);
                    s.setTextColor(th.textPrimary);
                }
                
                s.setTextSize(1);
                String t = currentTitles[titleIdx];
                if (t.length() > 50) t = t.substring(0, 47) + "...";
                drawWrappedText(s, t, 9, itemY + 4, 136, 2);
            }
            
            // Right Pane (55% width - 161px): Beautiful, recessed card with auto-wrapping large font
            UI::drawRecessedPanel(s, 154, 25, 161, 194, th);
            
            if (!currentTitles.empty()) {
                String selectedTitle = currentTitles[titleMenuIndex];
                s.setTextColor(th.accentBright);
                s.setTextSize(2);
                drawWrappedText(s, selectedTitle, 162, 35, 145, 6);
                
                s.drawLine(158, 175, 310, 175, th.border);
                
                s.setTextColor(th.textFaint);
                s.setTextSize(1);
                String domain = getDomain(feeds[feedMenuIndex].url);
                if (domain.length() > 22) domain = domain.substring(0, 19) + "...";
                s.setCursor(158, 180);
                s.print(domain);
                
                s.setCursor(158, 195);
                s.printf("Item %d of %d", titleMenuIndex + 1, (int)currentTitles.size());
            } else {
                s.setTextColor(th.textDim);
                s.setTextSize(1);
                s.setCursor(162, 35);
                s.print("No articles loaded.");
            }
            
            UI::drawFooter(s, th, "W/S to scroll articles, Del to back");
        }
    }

    void drawStatus(AppContext* ctx) override {
        auto& is = *ctx->intDisplay;
        const Theme& th = *ctx->theme;
        
        if (currentMode == RssMode::FEED_LIST) {
            // Ambient Dashboard: large transmitter tower & pulsing waves
            is.setTextColor(th.accentDim);
            is.setTextSize(1);
            is.setCursor(10, 8);
            is.print("AMBIENT DASHBOARD");
            is.drawLine(10, 18, 230, 18, th.border);
            
            int cx = 120;
            int cy = 75;
            
            int pulseTime = millis() / 25;
            int r1 = (pulseTime) % 50 + 5;
            int r2 = (pulseTime + 25) % 50 + 5;
            
            auto getPulseColor = [&](int radius) -> uint16_t {
                if (radius < 20) return th.accentBright;
                if (radius < 40) return th.accent;
                return th.accentDim;
            };
            
            is.drawCircle(cx, cy - 15, r1, getPulseColor(r1));
            is.drawCircle(cx, cy - 15, r2, getPulseColor(r2));
            
            // Tower structure
            is.drawLine(cx, cy - 15, cx - 15, cy + 30, th.textPrimary);
            is.drawLine(cx, cy - 15, cx + 15, cy + 30, th.textPrimary);
            is.drawLine(cx - 10, cy + 5, cx + 10, cy + 5, th.border);
            is.drawLine(cx - 13, cy + 18, cx + 13, cy + 18, th.border);
            
            is.drawLine(cx - 10, cy + 5, cx + 13, cy + 18, th.border);
            is.drawLine(cx + 10, cy + 5, cx - 13, cy + 18, th.border);
            is.drawLine(cx - 13, cy + 18, cx + 15, cy + 30, th.border);
            is.drawLine(cx + 13, cy + 18, cx - 15, cy + 30, th.border);
            
            // Blinking light
            bool lit = (millis() / 400) % 2 == 0;
            is.fillCircle(cx, cy - 15, 3, lit ? th.danger : th.accentBright);
            
            is.setTextColor(th.textFaint);
            is.setCursor(10, 120);
            is.print("System status: ACTIVE");
        }
        else if (currentMode == RssMode::FETCHING) {
            // Downloading container with large centered percentage ticker and animated scrollbar
            is.drawRect(15, 15, 210, 105, th.border);
            is.drawRect(18, 18, 204, 99, th.accentDim);
            
            is.setTextColor(th.textPrimary);
            is.setTextSize(1);
            is.setCursor(85, 28);
            is.print("DOWNLOADING");
            
            int pct = 0;
            if (!currentTitles.empty()) {
                pct = (currentTitles.size() * 100) / 15;
            }
            char pctStr[16];
            snprintf(pctStr, sizeof(pctStr), "%d%%", pct);
            
            is.setTextColor(th.accentBright);
            is.setTextSize(3);
            int textW = strlen(pctStr) * 18;
            is.setCursor(120 - textW / 2, 45);
            is.print(pctStr);
            
            // Animated scrollbar marquee
            int offset = (millis() / 40) % 30;
            is.drawRect(35, 90, 170, 8, th.border);
            for (int bx = 37 - 30 + offset; bx < 203; bx += 30) {
                int startX = max(37, bx);
                int endX = min(203, bx + 15);
                if (startX < endX) {
                    is.fillRect(startX, 92, endX - startX, 4, th.accentBright);
                }
            }
        }
        else if (currentMode == RssMode::READING) {
            // Big Progress Widget
            is.setTextColor(th.accentDim);
            is.setTextSize(1);
            is.setCursor(10, 8);
            is.print("ARTICLE STATUS");
            is.drawLine(10, 18, 230, 18, th.border);
            
            is.setTextColor(th.textDim);
            is.setTextSize(1);
            is.setCursor(100, 25);
            is.print("ARTICLE");
            
            int total = currentTitles.size();
            int current = titleMenuIndex + 1;
            char idxStr[32];
            snprintf(idxStr, sizeof(idxStr), "%02d / %02d", current, total);
            
            is.setTextColor(th.accentBright);
            is.setTextSize(3);
            int textW = strlen(idxStr) * 18;
            is.setCursor(120 - textW / 2, 40);
            is.print(idxStr);
            
            // Thick progress bar
            float ratio = (total > 0) ? (float)current / total : 0.0f;
            is.drawRect(20, 80, 200, 12, th.border);
            int fillW = (int)(196 * ratio);
            if (fillW > 0) {
                is.fillRect(22, 82, fillW, 8, th.accentBright);
            }
            
            // Pulsing packet animation
            is.drawLine(20, 110, 220, 110, th.border);
            int packTime = millis() / 8;
            for (int i = 0; i < 3; i++) {
                int px = 20 + ((packTime + i * 70) % 200);
                is.fillCircle(px, 110, 2, th.accentBright);
            }
        }
    }

    bool handleInput(AppContext* ctx, char key, bool isDel, bool isEnter) override {
        if (currentMode == RssMode::FEED_LIST) {
            int numFeeds = feeds.size();
            if (numFeeds > 0) {
                if (key == 'w' || key == ';') {
                    feedMenuIndex = (feedMenuIndex - 2 + numFeeds) % numFeeds;
                } else if (key == 's' || key == '.') {
                    feedMenuIndex = (feedMenuIndex + 2) % numFeeds;
                } else if (key == 'a' || key == ',' || key == '[') {
                    feedMenuIndex = (feedMenuIndex - 1 + numFeeds) % numFeeds;
                } else if (key == 'd' || key == '/' || key == ']') {
                    feedMenuIndex = (feedMenuIndex + 1) % numFeeds;
                }
                
                // Keep selected card in grid viewport (scroll rows)
                int activeRow = feedMenuIndex / 2;
                if (activeRow < scrollOffset) {
                    scrollOffset = activeRow;
                } else if (activeRow >= scrollOffset + 2) {
                    scrollOffset = activeRow - 1;
                }
            }
            
            if (isEnter) {
                if (!feeds.empty() && feeds[feedMenuIndex].url != "") {
                    String url = feeds[feedMenuIndex].url;
                    currentMode = RssMode::FETCHING;
                    
                    ctx->extSprite->fillScreen(ctx->theme->bg);
                    draw(ctx);
                    ctx->extSprite->pushSprite(0, 0);
                    
                    ctx->intDisplay->fillScreen(ctx->theme->bg);
                    drawStatus(ctx);
                    ((M5Canvas*)ctx->intDisplay)->pushSprite(0, 0);
                    
                    fetchFeed(ctx, url);
                }
            } else if (key == 27 || key == '`') {
                return false;
            }
        } 
        else if (currentMode == RssMode::READING) {
            if (key == 'w' || key == ';') {
                if (currentTitles.size() > 0) {
                    titleMenuIndex = (titleMenuIndex - 1 + currentTitles.size()) % currentTitles.size();
                }
            } else if (key == 's' || key == '.') {
                if (currentTitles.size() > 0) {
                    titleMenuIndex = (titleMenuIndex + 1) % currentTitles.size();
                }
            } else if (key == 27 || key == '`' || isDel) {
                currentMode = RssMode::FEED_LIST;
                scrollOffset = 0; // reset for grid viewport
            }
        }
        return true;
    }
};
