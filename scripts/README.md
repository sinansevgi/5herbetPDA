# 5herbet PDA Desktop Scripts

This folder contains utility scripts that are meant to be run on your **Mac or PC**, not on the M5Cardputer itself. These scripts process and generate the asset databases required for the applications to function correctly offline on the Cardputer.

---

## 1. Dictionary Setup (`generate_dict.py`)

The **Dictionary** app looks up words offline using a custom, binary-searchable index file. You must generate this file on your PC before using the app.

### What it does:
1. Downloads a lightweight, clean version of Webster's English Dictionary (~5.5MB JSON).
2. Sanitizes and sorts all word keys alphabetically.
3. Generates a compact index offset table followed by raw text definition payloads.
4. Saves this package as `dict.bin`. Because it is pre-sorted, the Cardputer can binary search the file on the fly with 0 bytes of RAM index overhead and sub-50ms lookup times!

### Running the script:
1. Open your terminal and navigate to the `scripts` folder:
   ```bash
   cd /Users/sinan/Projects/5herbetPDA/scripts
   ```
2. Run the generator script:
   ```bash
   python3 generate_dict.py
   ```
3. Copy the generated `dict.bin` file to your Cardputer's SD card at:
   ```text
   [SD Card Root]/5herbetPDA/dict.bin
   ```

---

## 2. Atlas Setup (`map_clicker.py`)

The **Atlas** app draws a **vector outline of the world map programmatically** directly on the screen. No background image is needed on your SD card!

However, the Cardputer still reads the location coordinates and metadata for cities and countries from a CSV file on the SD card.

### Build/Edit Location Coordinates:
To let the Cardputer know the exact X and Y screen pixel coordinates of cities and countries, you can map them visually:
1. Run the clicker tool on your PC (ensure you have a reference `map.jpg` or `world_map.jpg` in your script directory for the GUI to load):
   ```bash
   python3 map_clicker.py
   ```
2. A GUI window will open showing the world map. Click anywhere on the map to add a location:
   - Clicking a point will prompt you in the terminal for the location metadata (`Name`, `Capital`, `Population`, `Currency`, `Timezone Offset`).
   - The coordinates and metadata are saved automatically to a CSV database file.
3. Copy the completed coordinates database to your SD card at:
   ```text
   [SD Card Root]/5herbetPDA/Atlas/countries.csv
   ```
