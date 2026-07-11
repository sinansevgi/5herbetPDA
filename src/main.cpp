#include <Arduino.h>
#include <new>
#include "driver/gpio.h"
#include <M5Cardputer.h>
#include <SD.h>
#include <SPI.h>
#include <M5GFX.h>
#include <lgfx/v1/panel/Panel_LCD.hpp>
#include <WiFi.h>
#include <time.h>
#include "App.h"
#include "apps/DesktopApp.h"
#include "apps/WordApp.h"
#include "apps/CalculatorApp.h"
#include "apps/SysInfoApp.h"
#include "apps/FileExplorerApp.h"
#include "apps/SettingsApp.h"
#include "apps/AtlasApp.h"
#include "apps/CalendarApp.h"
#include "apps/UtilitiesApp.h"
#include "apps/RssApp.h"
#include <ArduinoJson.h>
#include "AudioSystem.h"

#define TFT_LED_PIN 15 // Connect external display LED/BLK pin to this GPIO pin (e.g., GPIO 15) to support software sleep
#define HALL_SENSOR_PIN 13 // G13 on Cardputer ADV expansion header for A3212EUA-T Hall Sensor

AudioSystem SysAudio;
std::vector<Theme> ALL_THEMES;
const Theme THEME_FALLBACK = { "LCD Cream", InteractionStyle::ORGANIZER, 0xF7BC, 0xF7BC, 0xDF5A, 0xE73C, 0x2BEC, 0x6B4D, 0x2BEC, 0x0000, 0x528A, 0x9CF3, 0x4208, 0x2C8C, 0x1C44, 0xC000, 0xC618, 0xAD75 };

const Theme DEFAULT_THEMES[] = {
    THEME_FALLBACK,
    { "Blue Steel", InteractionStyle::STEEL, 0x0841, 0x10a2, 0x0020, 0x18e3, 0x5bdf, 0x3b0e, 0x7bff, 0xdedb, 0x8c71, 0x528a, 0x4aef, 0x07e0, 0x0380, 0xc180, 0x1082, 0x2945 },
    { "Synthwave", InteractionStyle::VAPORWARE, 0x1805, 0x300B, 0x1003, 0x4811, 0x07FF, 0x03EF, 0xF81F, 0xFFFF, 0xB5B6, 0x6B4D, 0x8C71, 0x07FF, 0xF81F, 0xF800, 0x5014, 0x9015 },
    { "Hackerpunk", InteractionStyle::HACKERPUNK, 0x0000, 0x0800, 0x0000, 0x2000, 0xF800, 0x8000, 0xFFFF, 0xFFFF, 0xC000, 0x8000, 0x5000, 0xF800, 0x8000, 0xF800, 0x4000, 0x8000 },
    { "Matrix", InteractionStyle::MATRIX, 0x0100, 0x0180, 0x0080, 0x0200, 0x07e0, 0x0400, 0xafe0, 0x07e0, 0x0400, 0x0280, 0x05e0, 0x07e0, 0x03e0, 0xf800, 0x0200, 0x03e0 },
    { "Cassette Deck", InteractionStyle::CASSETTE, 0x1082, 0x18e3, 0x0841, 0x2104, 0x5edf, 0x349a, 0x7fff, 0xd6ba, 0x8410, 0x528a, 0xc511, 0xf800, 0x7800, 0xf800, 0x18e3, 0xce51 },
    { "Dusty Rose", InteractionStyle::PASTEL, 0xFFDE, 0xFFDE, 0xE6BA, 0xFFDF, 0xB1EC, 0x828C, 0xD14B, 0x0000, 0x4208, 0x8410, 0xA4B2, 0x85B2, 0x5C0D, 0xCB2B, 0xF69B, 0xD5F7 }
};
const int NUM_DEFAULT_THEMES = 7;

// =============================================================
// EXTERNAL DISPLAY — ILI9341 PANEL DRIVER
// =============================================================
class Panel_ILI9341_Local : public lgfx::v1::Panel_LCD {
public:
    Panel_ILI9341_Local(void) {
        _cfg.memory_width  = 240;
        _cfg.memory_height = 320;
        _cfg.panel_width  = 240;
        _cfg.panel_height = 320;
        _cfg.offset_x         = 0;
        _cfg.offset_y         = 0;
        _cfg.offset_rotation  = 0;
        _cfg.dummy_read_pixel = 8;
        _cfg.dummy_read_bits  = 1;
        _cfg.readable         = true;
        _cfg.invert           = false;
        _cfg.rgb_order        = false;
        _cfg.dlen_16bit       = false;
        _cfg.bus_shared       = true;
        _write_depth = lgfx::v1::rgb565_2Byte;
        _read_depth  = lgfx::v1::rgb888_3Byte;
    }
protected:
    uint8_t getMadCtl(uint8_t r) const override {
        static constexpr uint8_t madctl_table[] = {
            0x00, 0x20|0x40, 0x40|0x80, 0x20|0x80,
            0x80, 0x20|0x40|0x80, 0x40, 0x20
        };
        return madctl_table[r] | (_cfg.rgb_order ? 0x08 : 0x00);
    }
    const uint8_t* getInitCommands(uint8_t listno) const override {
        static constexpr uint8_t list0[] = {
            0x01, 0x80, 150,
            0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
            0xCF, 3, 0x00, 0xC1, 0x30,
            0xE8, 3, 0x85, 0x00, 0x78,
            0xEA, 2, 0x00, 0x00,
            0xED, 4, 0x64, 0x03, 0x12, 0x81,
            0xF7, 1, 0x20,
            0xC0, 1, 0x23, 0xC1, 1, 0x10,
            0xC5, 2, 0x3e, 0x28, 0xC7, 1, 0x86,
            0x36, 1, 0x48, 0x3A, 1, 0x55,
            0xB1, 2, 0x00, 0x18,
            0xB6, 3, 0x08, 0x82, 0x27,
            0xF2, 1, 0x00, 0x26, 1, 0x01,
            0xE0, 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
                       0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
            0xE1, 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
                       0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
            0x11, 0x80, 120,
            0x29, 0x80, 120,
            0xFF, 0xFF,
        };
        return (listno == 0) ? list0 : nullptr;
    }
};

class LGFX_ILI9341 : public lgfx::v1::LGFX_Device {
    Panel_ILI9341_Local panel;
    lgfx::v1::Bus_SPI   bus;
    lgfx::v1::Light_PWM light;
public:
    uint32_t getDisplayId() { return panel.readCommand(0x04, 0, 4); }
    LGFX_ILI9341() {
        auto b = bus.config();
        b.spi_host = SPI3_HOST; b.spi_mode = 0;
        b.freq_write = 40000000; b.freq_read = 16000000;
        b.spi_3wire = true; b.use_lock = true;
        b.dma_channel = 1;
        b.pin_sclk = 40; b.pin_mosi = 14; b.pin_miso = -1; b.pin_dc = 6;
        bus.config(b);
        panel.setBus(&bus);

        auto p = panel.config();
        p.pin_cs = 5; p.pin_rst = 3; p.pin_busy = -1;
        p.readable = false; p.invert = false; p.rgb_order = false;
        p.bus_shared = true;
        p.memory_width = 240; p.memory_height = 320;
        p.panel_width = 240; p.panel_height = 320;
        panel.config(p);

        #if defined(TFT_LED_PIN) && TFT_LED_PIN >= 0
        auto l = light.config();
        l.pin_bl = TFT_LED_PIN;
        l.invert = false;
        l.freq   = 12000;           // 12kHz PWM frequency
        l.pwm_channel = 7;          // Free LEDC channel
        light.config(l);
        panel.setLight(&light);
        #endif

        setPanel(&panel);
    }
};

// =============================================================
// GLOBAL OBJECTS
// =============================================================
LGFX_ILI9341 externalDisplay;
SPIClass sdSPI(HSPI);
M5Canvas extSprite(&externalDisplay);
M5Canvas intSprite(&M5Cardputer.Display);

AppContext appContext;
void AppContext::setExtBrightness(int b) { externalDisplay.setBrightness(b); }
App* currentAppInstance = nullptr;

unsigned long lastInputTime = 0;
bool isSleeping = false;
bool isScreenOff = false;

static unsigned long lastNTPAttempt = 0;
static bool wifiWasConnected = false;
static unsigned long lastFallbackUpdate = 0;

// =============================================================
// DYNAMIC THEMES
// =============================================================


void loadThemes() {
    ALL_THEMES.clear();
    
    // 1. Load default themes baked into firmware
    for (int i = 0; i < NUM_DEFAULT_THEMES; i++) {
        ALL_THEMES.push_back(DEFAULT_THEMES[i]);
    }

    // 2. Load custom themes from SD if available
    if (!appContext.sdAvailable) return;
    if (!SD.exists("/5herbetPDA/themes.json")) return;

    File f = SD.open("/5herbetPDA/themes.json", FILE_READ);
    if (!f) return;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err) return;

    JsonArray themes = doc["themes"];
    for (JsonObject t : themes) {
        Theme theme;
        theme.name[0] = '\0';
        {
            const char* n = t["name"].as<const char*>();
            if (n) strncpy(theme.name, n, sizeof(theme.name) - 1);
            theme.name[sizeof(theme.name) - 1] = '\0';
        }
        String styleStr = t["interactionStyle"].as<String>();
        if (styleStr == "STEEL") theme.interactionStyle = InteractionStyle::STEEL;
        else if (styleStr == "VAPORWARE") theme.interactionStyle = InteractionStyle::VAPORWARE;
        else if (styleStr == "HACKERPUNK") theme.interactionStyle = InteractionStyle::HACKERPUNK;
        else if (styleStr == "MATRIX") theme.interactionStyle = InteractionStyle::MATRIX;
        else if (styleStr == "CASSETTE") theme.interactionStyle = InteractionStyle::CASSETTE;
        else if (styleStr == "PASTEL") theme.interactionStyle = InteractionStyle::PASTEL;
        else theme.interactionStyle = InteractionStyle::ORGANIZER;
        
        theme.bg = (uint16_t)strtol(t["bg"].as<const char*>(), NULL, 16);
        theme.bgPanel = (uint16_t)strtol(t["bgPanel"].as<const char*>(), NULL, 16);
        theme.bgRecessed = (uint16_t)strtol(t["bgRecessed"].as<const char*>(), NULL, 16);
        theme.bgRaised = (uint16_t)strtol(t["bgRaised"].as<const char*>(), NULL, 16);
        theme.accent = (uint16_t)strtol(t["accent"].as<const char*>(), NULL, 16);
        theme.accentDim = (uint16_t)strtol(t["accentDim"].as<const char*>(), NULL, 16);
        theme.accentBright = (uint16_t)strtol(t["accentBright"].as<const char*>(), NULL, 16);
        theme.textPrimary = (uint16_t)strtol(t["textPrimary"].as<const char*>(), NULL, 16);
        theme.textDim = (uint16_t)strtol(t["textDim"].as<const char*>(), NULL, 16);
        theme.textFaint = (uint16_t)strtol(t["textFaint"].as<const char*>(), NULL, 16);
        theme.steel = (uint16_t)strtol(t["steel"].as<const char*>(), NULL, 16);
        theme.indicator = (uint16_t)strtol(t["indicator"].as<const char*>(), NULL, 16);
        theme.indicatorDim = (uint16_t)strtol(t["indicatorDim"].as<const char*>(), NULL, 16);
        theme.danger = (uint16_t)strtol(t["danger"].as<const char*>(), NULL, 16);
        theme.selectBg = (uint16_t)strtol(t["selectBg"].as<const char*>(), NULL, 16);
        theme.border = (uint16_t)strtol(t["border"].as<const char*>(), NULL, 16);
        ALL_THEMES.push_back(theme);
    }
}

// =============================================================
// SETTINGS PERSISTENCE
// =============================================================
void loadSettings() {
    if (!appContext.sdAvailable) return;
    File f = SD.open("/settings.conf", FILE_READ);
    if (!f) return;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        int eq = line.indexOf('=');
        if (eq < 0) continue;
        String key = line.substring(0, eq);
        String val = line.substring(eq + 1);
        if      (key == "SSID")     appContext.wifiSSID = val;
        else if (key == "PASS")     appContext.wifiPass = val;
        else if (key == "SCREEN")   appContext.screenTimeoutMins = val.toInt();
        else if (key == "SLEEP")    appContext.sleepTimeoutMins  = val.toInt();
        else if (key == "TZ")       appContext.timezoneOffsetHours = val.toInt();
        else if (key == "USERNAME") appContext.userName = val;
        else if (key == "THEME")    appContext.setTheme(val.toInt());
        else if (key == "VOL")      SysAudio.setVolume(val.toInt() * 255 / 100);
        else if (key == "BRIGHTNESS") {
            appContext.screenBrightness = val.toInt();
            appContext.setExtBrightness(appContext.screenBrightness);
            M5Cardputer.Display.setBrightness(appContext.screenBrightness);
        }
    }
    f.close();
}

// =============================================================
// NTP TIME SYNC
// =============================================================
void syncNTP() {
    if (WiFi.status() != WL_CONNECTED) return;
    appContext.showNotification("Syncing time...");
    if (currentAppInstance) currentAppInstance->draw(&appContext);
    if (appContext.extScreenConnected && appContext.theme) {
        extSprite.fillRect(0, 220, 320, 20, appContext.theme->accent);
        extSprite.setTextColor(appContext.theme->bg);
        extSprite.setTextSize(1);
        extSprite.setCursor(4, 226);
        extSprite.print(appContext.notificationMsg.c_str());
        extSprite.pushSprite(0, 0);
    } else {
        intSprite.fillScreen(appContext.theme->bg);
        intSprite.setTextColor(appContext.theme->accent);
        intSprite.setCursor(10, 60);
        intSprite.print("Syncing time...");
        intSprite.pushSprite(0, 0);
    }
    
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;
    unsigned long deadline = millis() + 5000;
    while (millis() < deadline) {
        if (getLocalTime(&timeinfo)) {
            time_t now = mktime(&timeinfo);
            appContext.epochBase   = (unsigned long)now;
            appContext.epochMillis = millis();
            appContext.timesynced  = true;
            appContext.showNotification("NTP time synced");
            return;
        }
        delay(200);
    }
    appContext.showNotification("NTP sync failed");
}

// =============================================================
// APP LIFECYCLE
// =============================================================
void switchApp() {
    if (appContext.extScreenConnected && appContext.theme) {
        for (int i = 0; i < 8; i++) {
            extSprite.fillScreen(appContext.theme->bg);
            UI::drawLoading(extSprite, *appContext.theme, i);
            extSprite.pushSprite(0, 0);
            
            if (intSprite.getBuffer()) {
                intSprite.fillScreen(appContext.theme->bg);
                intSprite.setTextColor(appContext.theme->accent);
                intSprite.setTextSize(1);
                intSprite.setCursor(10, 60);
                intSprite.print("Loading app...");
                for (int d = 0; d <= i % 4; d++) {
                    intSprite.print(".");
                }
                intSprite.pushSprite(0, 0);
            }

            SysAudio.update();
            delay(50);
        }
    }

    if (currentAppInstance) { delete currentAppInstance; currentAppInstance = nullptr; }
    switch (appContext.nextApp) {
        case AppType::DESKTOP:      currentAppInstance = new DesktopApp();      break;
        case AppType::NOTEPAD:      currentAppInstance = new WordApp();      break;
        case AppType::CALCULATOR:   currentAppInstance = new CalculatorApp();   break;
        case AppType::CALENDAR:     currentAppInstance = new CalendarApp();     break;
        case AppType::SYSINFO:      currentAppInstance = new SysInfoApp();      break;
        case AppType::FILEEXPLORER: currentAppInstance = new FileExplorerApp(); break;
        case AppType::SETTINGS:     currentAppInstance = new SettingsApp();     break;
        case AppType::ATLAS:        currentAppInstance = new AtlasApp();        break;
        case AppType::UTILITIES:    currentAppInstance = new UtilitiesApp();    break;
        case AppType::RSSREADER:    currentAppInstance = new RssApp();          break;
        default:                    currentAppInstance = new DesktopApp();      break;
    }
    String args = appContext.nextAppArgs;
    appContext.nextApp = AppType::NONE;
    appContext.nextAppArgs = "";
    if (currentAppInstance) currentAppInstance->setup(&appContext, args);
}

// =============================================================
// DAEMONS
// =============================================================
void runDaemons() {
    bool nowConnected = (WiFi.status() == WL_CONNECTED);
    if (nowConnected && (!wifiWasConnected || millis() - lastNTPAttempt > 1800000UL)) {
        lastNTPAttempt = millis();
        syncNTP();
    }
    wifiWasConnected = nowConnected;

    // Alarms Check
    if (appContext.timesynced) {
        unsigned long nowEpoch = appContext.epochBase + (millis() - appContext.epochMillis) / 1000UL;
        
        static unsigned long lastTimeSave = 0;
        if (millis() - lastTimeSave > 3600000UL) { // 1 hour
            lastTimeSave = millis();
            if (appContext.sdAvailable) {
                File f = SD.open("/5herbetPDA/last_time.txt", FILE_WRITE);
                if (f) {
                    f.println(nowEpoch);
                    f.close();
                }
            }
        }
        long localEpoch = (long)nowEpoch + (long)appContext.timezoneOffsetHours * 3600L;
        int h = ((localEpoch / 3600) % 24 + 24) % 24;
        int m = ((localEpoch / 60) % 60 + 60) % 60;
        
        // Reset hasRungToday at midnight (or close to it)
        if (h == 0 && m == 0) {
            for (auto& a : appContext.alarms) {
                a.hasRungToday = false;
            }
        }

        for (auto& a : appContext.alarms) {
            if (a.enabled && a.hour == h && a.minute == m && !a.hasRungToday) {
                a.hasRungToday = true;
                appContext.alarmRinging = true;
                appContext.alarmRingStart = millis();
            }
        }
    }

    if (appContext.alarmRinging) {
        if (millis() - appContext.alarmRingStart > 60000) {
            appContext.alarmRinging = false; // Auto-dismiss after 60s
        } else if ((millis() / 500) % 2 == 0) { // Beep every second
            SysAudio.play(appContext.theme->interactionStyle, SoundEvent::ALERT);
        }
    }
}

// =============================================================
// FALLBACK DASHBOARD (no external screen)
// =============================================================
void drawFallbackDashboard() {
    const Theme& th = *appContext.theme;
    intSprite.fillScreen(th.bg);

    // Header Panel (0 to 20px)
    intSprite.fillRect(0, 0, 240, 20, th.bgRaised);
    intSprite.drawLine(0, 20, 239, 20, th.border);
    intSprite.setTextColor(th.accentBright);
    intSprite.setTextSize(1);
    intSprite.setCursor(6, 6);
    intSprite.print("5HERBET PDA");

    // Time in Header
    char timeBuf[8];
    appContext.getTime(timeBuf, sizeof(timeBuf));
    intSprite.setTextColor(th.accent);
    intSprite.setCursor(200, 6);
    intSprite.print(timeBuf);

    // Divider Line (vertical separator between Clock/Date and Stats Grid)
    intSprite.drawLine(135, 26, 135, 110, th.border);

    // Left Column: Giant Clock & Date
    intSprite.setTextColor(th.textPrimary);
    intSprite.setTextSize(3);
    intSprite.setCursor(10, 32);
    intSprite.print(timeBuf); // "HH:MM"

    // Date display below clock
    char dateBuf[8];
    appContext.getDateDDMM(dateBuf, sizeof(dateBuf));
    char yearBuf[6];
    appContext.getYear(yearBuf, sizeof(yearBuf));
    intSprite.setTextColor(th.textDim);
    intSprite.setTextSize(1);
    intSprite.setCursor(10, 62);
    intSprite.printf("%s.%s", dateBuf, yearBuf);

    // Mode subtitle
    intSprite.setTextColor(th.textFaint);
    intSprite.setCursor(10, 82);
    intSprite.print("FALLBACK MODE");
    intSprite.setCursor(10, 94);
    intSprite.print("No Ext Screen");

    // Right Column: System Status Widgets (arranged neatly from x=145)
    int bat = M5Cardputer.Power.getBatteryLevel();
    bool wifiUp = (WiFi.status() == WL_CONNECTED);

    // Battery Widget
    intSprite.setTextColor(th.textFaint);
    intSprite.setCursor(145, 28);
    intSprite.print("BAT");
    intSprite.setTextColor(th.textPrimary);
    intSprite.setCursor(185, 28);
    intSprite.printf("%d%%", bat);
    // Draw battery fill gauge
    intSprite.drawRect(145, 40, 80, 8, th.border);
    int filledWidth = (76 * bat) / 100;
    if (filledWidth < 0) filledWidth = 0;
    if (filledWidth > 76) filledWidth = 76;
    intSprite.fillRect(147, 42, filledWidth, 4, th.accent);

    // WiFi Widget
    intSprite.setTextColor(th.textFaint);
    intSprite.setCursor(145, 58);
    intSprite.print("NET");
    intSprite.setTextColor(wifiUp ? th.indicator : th.danger);
    intSprite.setCursor(185, 58);
    intSprite.print(wifiUp ? "ONLINE" : "OFFLINE");
    UI::drawLED(intSprite, 222, 60, wifiUp ? th.indicator : th.danger);

    // SD Widget
    intSprite.setTextColor(th.textFaint);
    intSprite.setCursor(145, 78);
    intSprite.print("SD");
    intSprite.setTextColor(appContext.sdAvailable ? th.indicator : th.textDim);
    intSprite.setCursor(185, 78);
    intSprite.print(appContext.sdAvailable ? "READY" : "NO SD");
    UI::drawLED(intSprite, 222, 80, appContext.sdAvailable ? th.indicator : th.border);

    // Notification Banner (if any) or standard bottom guide
    if (millis() - appContext.notificationTime < 5000 && appContext.notificationMsg != "") {
        intSprite.fillRect(0, 112, 240, 23, th.accent);
        intSprite.setTextColor(th.bg);
        intSprite.setTextSize(1);
        int textWidth = appContext.notificationMsg.length() * 6;
        int startX = (240 - textWidth) / 2;
        if (startX < 4) startX = 4;
        intSprite.setCursor(startX, 120);
        intSprite.print(appContext.notificationMsg.c_str());
    } else {
        intSprite.drawLine(0, 114, 239, 114, th.border);
        intSprite.setTextColor(th.textFaint);
        intSprite.setCursor(10, 122);
        intSprite.print("Sys standby...");
        uint32_t pulseColor = ((millis() / 500) % 2 == 0) ? th.accent : th.accentDim;
        UI::drawLED(intSprite, 222, 122, pulseColor);
    }
}

// =============================================================
// SETUP & LOOP
// =============================================================
void setup() {
    Serial.begin(115200);

    pinMode(HALL_SENSOR_PIN, INPUT_PULLUP);

    // Reset pins 39 (MISO) and 40 (SCLK) to disable JTAG functionality and reclaim them as standard GPIOs.
    // This is critical because GPIO 3 is the display Reset pin, which is also an ESP32-S3 strapping pin.
    // If GPIO 3 is LOW/floating at boot (e.g. during a software reset from a launcher), the ESP32-S3 
    // hardware JTAG block will automatically route external JTAG to GPIO 39-42, disabling SPI on them.
    gpio_reset_pin((gpio_num_t)39);
    gpio_reset_pin((gpio_num_t)40);

    // Disable SD CS (GPIO 12) and external display CS (GPIO 5) immediately on startup
    // to prevent SPI bus contention. This is crucial when the app is booted from
    // a launcher like M5Launcher, which leaves the SD card in an active/selected state.
    pinMode(12, OUTPUT);
    digitalWrite(12, HIGH);
    pinMode(5, OUTPUT);
    digitalWrite(5, HIGH);

    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1);

    // Backlight is handled by lgfx::Light_PWM internally

    // Explicitly enable external 5V power output to power the external screen.
    // M5Launcher typically disables external output to save power, leaving the screen unpowered.
    M5Cardputer.Power.setExtOutput(true);
    delay(100); // Wait for the power rail to stabilize

    // Initialize SPI bus on SPI3_HOST (HSPI) with no default SS pin (passed -1) to prevent the hardware
    // controller from driving GPIO 12 during general SPI operations.
    sdSPI.begin(40, 39, 14, -1);

    // Send 80 dummy clock cycles (10 bytes of 0xFF) at a safe 400 kHz
    // with both CS lines held HIGH. This forces the SD card's internal SPI
    // state machine to reset and release the MISO line (GPIO 39), resolving
    // bus contention caused by M5Launcher's boot sequence before SD.begin is run.
    sdSPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
    for (int i = 0; i < 10; i++) {
        sdSPI.transfer(0xFF);
    }
    sdSPI.endTransaction();

    // Initialize the SD card first to reset its SPI state machine and gracefully deselect it.
    // By running SD.begin before detecting the screen, the official SD library completes
    // the card initialization protocol, which automatically exits any active state
    // and holds CS (GPIO 12) HIGH.
    if (SD.begin(12, sdSPI, 40000000)) {
        Serial.println("SD Card initialized successfully!");
        appContext.sdAvailable = true;
        if (!SD.exists("/5herbetPDA"))       SD.mkdir("/5herbetPDA");
        if (!SD.exists("/5herbetPDA/Notes")) SD.mkdir("/5herbetPDA/Notes");
        if (!SD.exists("/5herbetPDA/organizer")) SD.mkdir("/5herbetPDA/organizer");
        loadThemes();
        loadSettings();
        
        if (SD.exists("/5herbetPDA/last_time.txt")) {
            File f = SD.open("/5herbetPDA/last_time.txt", FILE_READ);
            if (f) {
                String ts = f.readStringUntil('\n');
                ts.trim();
                unsigned long savedEpoch = ts.toInt();
                if (savedEpoch > 1700000000UL) { // Basic sanity check
                    appContext.epochBase = savedEpoch;
                    appContext.epochMillis = millis();
                    appContext.timesynced = true;
                }
                f.close();
            }
        }
        
        appContext.showNotification("SD card ready");
    } else {
        Serial.println("SD Card initialization FAILED!");
        appContext.sdAvailable = false;
        // SD card failed or is absent. Ensure CS pin is pulled HIGH so it doesn't float.
        pinMode(12, OUTPUT);
        digitalWrite(12, HIGH);
        loadThemes();
        appContext.showNotification("No SD card");
    }

    // Hardware reset the external display to ensure it starts in a clean state now that it has power
    pinMode(3, OUTPUT);
    digitalWrite(3, LOW);
    delay(50);
    digitalWrite(3, HIGH);
    delay(100);

    intSprite.deleteSprite();
    new (&intSprite) M5Canvas(&M5Cardputer.Display);
    intSprite.setColorDepth(8);
    intSprite.createSprite(240, 135);

    delay(800);

    // Initialize external display
    bool displayOk = false;
    for (int i = 0; i < 5; i++) {
        if (externalDisplay.init()) {
            displayOk = true;
            break;
        }
        delay(200);
    }

    // Force fallback mode ONLY if the user holds down the StampS3 button (BtnA) at boot
    bool forcedFallback = false;
    M5Cardputer.update();
    if (M5Cardputer.BtnA.isPressed()) {
        displayOk = false;
        forcedFallback = true;
        Serial.println("Fallback mode forced by user (BtnA pressed on boot).");
    }

    if (!displayOk) {
        appContext.extScreenConnected = false;
        Serial.println("External screen not detected or disabled.");
    } else {
        appContext.extScreenConnected = true;
        externalDisplay.setRotation(5);
        externalDisplay.fillScreen(0x0000);
        extSprite.setColorDepth(8);
        extSprite.createSprite(320, 240);
    }

    appContext.epochBase = 0;
    appContext.epochMillis = 0;
    WiFi.mode(WIFI_STA);
    if (appContext.wifiSSID.length() > 0) {
        WiFi.begin(appContext.wifiSSID.c_str(), appContext.wifiPass.c_str());
    }

    appContext.extSprite = &extSprite;
    appContext.intDisplay = &intSprite;
    
    if (appContext.extScreenConnected) {
        UI::drawStartupAnimation(extSprite, intSprite, *appContext.theme);
    } else {
        intSprite.fillScreen(appContext.theme->bg);
        intSprite.setTextColor(appContext.theme->danger);
        intSprite.setTextSize(1);
        intSprite.setCursor(4, 4);
        intSprite.println("WARNING: No External Screen");
        intSprite.setTextColor(appContext.theme->textPrimary);
        intSprite.setCursor(4, 20);
        intSprite.println("This firmware is designed");
        intSprite.setCursor(4, 30);
        intSprite.println("for an external display.");
        intSprite.setCursor(4, 40);
        intSprite.println("It will not work properly.");
        
        intSprite.setCursor(4, 60);
        intSprite.println("You can only view the task list,");
        intSprite.setCursor(4, 70);
        intSprite.println("mark them completed, and snooze/");
        intSprite.setCursor(4, 80);
        intSprite.println("turn off alarms without a screen.");
        
        intSprite.setTextColor(appContext.theme->accent);
        intSprite.setCursor(4, 110);
        intSprite.println("Press any key to continue...");
        intSprite.pushSprite(0, 0);

        unsigned long lastCheck = 0;
        while (true) {
            M5Cardputer.update();
            if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
                break;
            }
            
            if (!forcedFallback && millis() - lastCheck > 1000) {
                lastCheck = millis();
                if (externalDisplay.init()) {
                    appContext.extScreenConnected = true;
                    externalDisplay.setRotation(5);
                    externalDisplay.fillScreen(0x0000);
                    extSprite.setColorDepth(8);
                    extSprite.createSprite(320, 240);
                    UI::drawStartupAnimation(extSprite, intSprite, *appContext.theme);
                    break;
                }
            }
            delay(10);
        }
    }
    
    appContext.requestAppSwitch(AppType::DESKTOP);

    SysAudio.init();
    SysAudio.play(appContext.theme->interactionStyle, SoundEvent::BOOT);

    Serial.println("5herbet PDA started");
}

void loop() {
    M5Cardputer.update();

    unsigned long currentMillis = millis();
    unsigned long idleTime = currentMillis - lastInputTime;

    static bool wasLidClosed = false;
    bool lidClosed = (digitalRead(HALL_SENSOR_PIN) == LOW);
    if (lidClosed) wasLidClosed = true;

    // Power management
    bool sleepTriggered = isSleeping || lidClosed || (appContext.sleepTimeoutMins > 0 && idleTime > (unsigned long)appContext.sleepTimeoutMins * 60000UL);
    if (millis() > 2000 && M5Cardputer.BtnA.wasPressed()) {
        if (isSleeping) {
            // Wake up handled below
        } else if (isScreenOff) {
            // Just wake screen up
            lastInputTime = millis();
        } else {
            // Force sleep
            sleepTriggered = true;
        }
    }

    if (sleepTriggered) {
        if (!isSleeping) {
            SysAudio.play(appContext.theme->interactionStyle, SoundEvent::SHUTDOWN);
            for (int i=0; i<40; i++) { SysAudio.update(); delay(10); } // Wait for sound
            isSleeping = true;
            if (!isScreenOff) {
                M5Cardputer.Display.sleep();
                if (appContext.extScreenConnected) {
                    externalDisplay.sleep();
                    M5Cardputer.Power.setExtOutput(false);
                    // Backlight turns off automatically via externalDisplay.sleep()
                }
                isScreenOff = true;
            }
            M5Cardputer.Speaker.end(); // Stop speaker to prevent sleeping noise!
        }
        
        esp_sleep_enable_timer_wakeup(500000);
        esp_light_sleep_start();
        M5Cardputer.update();
        
        bool currentLidClosed = (digitalRead(HALL_SENSOR_PIN) == LOW);
        
        if (!currentLidClosed && wasLidClosed) {
            wasLidClosed = false; // Reset tracker
            M5Cardputer.Speaker.begin(); // Reinitialize speaker!
            SysAudio.init();             // Restore volume settings!
            lastInputTime = millis();
            isSleeping = false;
        } else if (!currentLidClosed && (M5Cardputer.Keyboard.isPressed() || M5Cardputer.BtnA.isPressed())) {
            M5Cardputer.Speaker.begin();
            SysAudio.init();
            lastInputTime = millis();
            isSleeping = false;
        } else {
            return;
        }
    }
    if (appContext.screenTimeoutMins > 0 && idleTime > (unsigned long)appContext.screenTimeoutMins * 60000UL) {
        if (!isScreenOff) {
            M5Cardputer.Display.sleep();
            if (appContext.extScreenConnected) {
                externalDisplay.sleep();
                M5Cardputer.Power.setExtOutput(false);
                // Backlight turns off automatically via externalDisplay.sleep()
            }
            isScreenOff = true;
        }
    } else if (isScreenOff) {
        M5Cardputer.Display.wakeup();
        M5Cardputer.Display.setBrightness(128);
        if (appContext.extScreenConnected) {
            M5Cardputer.Power.setExtOutput(true);
            // Backlight turns on automatically via externalDisplay.init() or wakeup()
            delay(100);
            
            // Hardware reset external display to guarantee clean restart
            pinMode(3, OUTPUT);
            digitalWrite(3, LOW);
            delay(50);
            digitalWrite(3, HIGH);
            delay(100);

            externalDisplay.init();
            externalDisplay.setRotation(5);
            externalDisplay.fillScreen(0x0000);
            if (currentAppInstance) {
                currentAppInstance->draw(&appContext);
            }
        }
        isScreenOff = false;
    }

    // App switching
    if (appContext.extScreenConnected && appContext.nextApp != AppType::NONE) {
        switchApp();
    }

    // Input
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        lastInputTime = millis();
        auto keys = M5Cardputer.Keyboard.keysState();
        
        if (appContext.alarmRinging) {
            appContext.alarmRinging = false; // Any key dismisses alarm
        } else {
            bool handled = false;
            if (currentAppInstance) {
            char c = 0;
            for (auto i : keys.word) { c = i; break; }
            if (c == 0 && M5Cardputer.Keyboard.isKeyPressed(27)) c = 27;
            handled = currentAppInstance->handleInput(&appContext, c, keys.del, keys.enter);
        }
        
            if (!handled) {
                if (M5Cardputer.Keyboard.isKeyPressed('`') || M5Cardputer.Keyboard.isKeyPressed(27)) {
                    if (appContext.extScreenConnected) appContext.requestAppSwitch(AppType::DESKTOP);
                }
            }
        }
    }

    // Render
    if (appContext.extScreenConnected) {
        if (currentAppInstance) {
            if (extSprite.getBuffer()) {
                currentAppInstance->draw(&appContext);
                extSprite.pushSprite(0, 0);
            }
            if (intSprite.getBuffer()) {
                intSprite.fillScreen(appContext.theme->bg);
                currentAppInstance->drawStatus(&appContext);
                intSprite.pushSprite(0, 0);
            }
        }
    } else {
        if (millis() - lastFallbackUpdate > 1000) {
            if (intSprite.getBuffer()) {
                drawFallbackDashboard();
                intSprite.pushSprite(0, 0);
            }
            lastFallbackUpdate = millis();
        }
    }

    runDaemons();
    SysAudio.update();
    delay(30);
}