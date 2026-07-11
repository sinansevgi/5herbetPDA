# 5herbet PDA 📟

An advanced, dual-screen personal digital assistant firmware designed for the **M5Stack Cardputer** (ESP32-S3) paired with an external SPI display. 

> [!WARNING]
> **Use at Your Own Risk!**
> This application was created primarily as a test project to explore dual-screen rendering and layout paradigms on the M5Stack Cardputer. Significant portions of the codebase were "vibe coded" (rapidly prototyped with generative AI assistance). It is highly experimental, still buggy, and lacks long-term testing.

---

## 🖥️ Dual-Screen UX Paradigm

To overcome the size limits of the built-in screen, this project splits duties across two displays:
1. **Tiny Internal Screen (1.14" LCD, 240×135)**: Acts as an ambient telemetry dashboard. It displays system health, active dial widgets, keyboard modifier status (Caps Lock/Shift/Alt), large operators, and large clocks.
2. **Large External Screen (320×240 TFT)**: Works as the primary canvas for active engagement. It renders application launchers, lists, text editors, vector maps, and settings panels.

If no external display is detected, the firmware enters **Fallback Mode**, rendering an optimized, single-screen retro dashboard on the internal display.

---

## 🔌 Hardware Pinout & Wiring Configuration

This project's display pinout is exactly identical to the popular [AndyAiCardputer/zx-spectrum-cardputer-ili9341](https://github.com/AndyAiCardputer/zx-spectrum-cardputer-ili9341) project. The screen backlight (LED/BLK) pin is connected directly to VCC (PIN 15) for permanent illumination.

To hook up the external ILI9341 SPI display, connect the display to the 14-pin 2.54mm expansion header on the back of the **Cardputer ADV** using the mapping below:

### Pinout Configuration Table
| ILI9341 Pin / Component | EXT Header Pin (Cardputer ADV) | GPIO | Description / Notes |
| :--- | :--- | :--- | :--- |
| **VCC** | PIN 15 | - | 3.3V Power |
| **GND** | PIN 11 | - | Ground |
| **CS** | PIN 13 | G5 | Chip Select |
| **RST / RESET** | PIN 1 | G3 | Reset |
| **DC / RS** | PIN 5 | G6 | Data / Command |
| **SDI / MOSI** | PIN 9 | G14 | SPI Data In |
| **SCK / CLK** | PIN 7 | G40 | SPI Clock |
| **LED / BLK** | - | G15 | **WARNING:** Adjustable Backlight. See safety note below! |
| **Hall Effect Sensor** | - | G13 | A3212EUA-T Data Pin for magnetic lid sleep |

> [!CAUTION]
> **HARDWARE SAFETY WARNING: LED/BLK Pin**
> Standard ESP32-S3 GPIO pins can only safely supply ~40mA. An ILI9341 backlight can draw 60mA–100mA. 
> **Before connecting the LED pin to G15**, you MUST check the back of your display PCB. If you see a small transistor (often labeled `J3Y` or `Q1`) near the pins, it is 100% safe to connect directly to the GPIO. If there is **no transistor**, connecting it directly to G15 will eventually burn out your Cardputer. You must use an external NPN transistor or MOSFET if your display lacks one!

> [!NOTE]
> **Hall Effect Sensor (Lid Sleep)**
> You can connect a Hall Effect sensor (like the A3212EUA-T) to magnetically detect when the PDA "lid" is closed, automatically sleeping the device. Connect the Sensor's `VCC` to 3.3V, `GND` to GND, and the `DATA` pin to **G13** on the expansion header.

---

## 🚀 Built-in Applications

- **📁 Desktop**: Left-pane application launcher (grid layout) combined with a right-pane "Today" widget panel showing current date, tasks, and system statuses.
- **📝 Word**: Distraction-free text processor with custom cursor tracking and text-formatting options.
- **📰 RSS Reader**: Offline headline scanner parsed directly from RSS XML feeds.
- **🧮 Calculator**: Retro calculator screen featuring a scrolling calculation "paper tape" tape history.
- **📅 Calendar**: Full-featured personal organizer including a Planner, Tasks Tracker, Habits Streak tracker, and Goals sheet.
- **📁 File Explorer**: SD card directory navigation system supporting creating, renaming, and deleting files and folders.
- **🗺️ Atlas**: Programmatic vector world map rendering. Allows scrolling a crosshair to inspect country capitals, populations, currencies, and local times.
- **🛠️ Utilities**: A suite of tools containing a Stopwatch, Countdown Timer, Alarms, Pomodoro Focus clock, and a local Web UI Server.
- **📖 Dictionary**: Ultra-fast offline word search using a custom pre-sorted index search (sub-50ms lookups).
- **⚙️ Settings**: Configuration panel for screen timeouts, timezone, audio volume, Wi-Fi credentials, and visual themes.

---

## 🎨 Themes

The UI features several custom, high-contrast visual themes:
* **Synthwave**: Vibrant neon purple, pink, and cyan accents.
* **Hackerpunk**: Toxic green terminal font over dark pitch-black backgrounds.
* **Matrix**: Monochromatic cascading greens.
* **Cassette**: Warm retro browns, oranges, and yellows.
* **Steel**: Clean corporate grays and industrial blue accents.
* **Vaporware**: Sunset orange and magenta gradients.

---

## 🛠️ Hardware Requirements

* **M5Stack Cardputer** (with ESP32-S3 and physical keyboard).
* **External SPI display** (320×240 TFT) supported by LovyanGFX/M5GFX.
* **3D Printed Case**: You can print a custom case designed to hold the Cardputer ADV and an external TFT display. This project uses the case design from:
  - [Prokuon/CardputerADV_Cap_TFT-2.8](https://github.com/Prokuon/CardputerADV_Cap_TFT-2.8) - 3D printable shell layout for the Cardputer ADV holding a 2.8" capacitive TFT screen.
* **FAT32 formatted MicroSD card** for database storage and settings.

---

## 💾 SD Card Directory Structure

To run all apps correctly offline, your SD card should be organized as follows:

```text
[SD Card Root]
├── settings.conf                 # Global settings configuration
└── 5herbetPDA/
    ├── dict.bin                  # Binary dictionary database (optional)
    ├── Atlas/
    │   └── countries.csv         # Custom Atlas coordinates (optional)
    ├── Calendar/                 # Directory containing organizer database files
    └── Notes/                    # Directory for Word processor text documents
```

### Format of `settings.conf`:
```ini
SSID=YourWiFiSSID
PASS=YourWiFiPassword
SCREEN=5
SLEEP=10
TZ=3
USERNAME=STRANGER
THEME=0
VOL=50
```

---

## ⚙️ Offline Desktop Scripts

Located in the [scripts/](./scripts) folder, these tools should be run on a Mac/PC to prepare offline resources:

### 1. Dictionary Compiler (`generate_dict.py`)
Downloads a lightweight copy of Webster's English Dictionary and builds a custom, sorted binary database. This allows the Cardputer to look up definitions with a binary search in milliseconds under zero memory overhead.
* **Run**:
  ```bash
  cd scripts
  python3 -m pip install requests
  python3 generate_dict.py
  ```
* Move the output `dict.bin` to `/5herbetPDA/dict.bin` on your SD card.

### 2. Atlas Location Mapper (`map_clicker.py`)
An interactive Tkinter desktop GUI for coordinates mapping. Allows you to click places on the template world map and enter metadata, appending coordinates directly to a `countries.csv` database.
* **Run**:
  ```bash
  cd scripts
  python3 map_clicker.py
  ```
* Move the output `countries.csv` to `/5herbetPDA/Atlas/countries.csv` on your SD card.

---

## 🛠️ Compilation and Upload

Ensure you have **PlatformIO** installed (VS Code extension or CLI).

1. Clone this repository.
2. Connect your M5Stack Cardputer via USB-C.
3. Compile and upload:
   ```bash
   pio run -t upload
   ```
4. Open the serial monitor:
   ```bash
   pio device monitor
   ```

---

## 💖 Credits & Inspirations

This project is built on top of and inspired by several outstanding open-source projects in the ESP32 and M5Stack community:
- [AndyAiCardputer/zx-spectrum-cardputer-ili9341](https://github.com/AndyAiCardputer/zx-spectrum-cardputer-ili9341): Used to understand how to connect and drive the external ILI9341 display.
- [bomberman30/AdvanceOS-for-cardputer](https://github.com/bomberman30/AdvanceOS-for-cardputer): Inspired software features and OS-style application functionalities.
- [nishad2m8/PDAputer](https://github.com/nishad2m8/PDAputer): Inspired application ideas and the offline PDA-style utility suite.
- **Bruce Firmware**: Inspired the multi-app portable utility layout style and modular firmware approach on the Cardputer.
- **M5Unified & LovyanGFX**: Powering the high-performance graphics, screen rotations, and smooth dual-display rendering engines.
- **M5Cardputer Library**: Providing low-level keyboard matrix, sound audio, and hardware mappings.
