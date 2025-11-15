# Additional Instructions and Reference

This document contains detailed information, troubleshooting guides, and advanced configuration options that supplement the main README.

## Table of Contents

1. [Detailed PlatformIO Setup](#detailed-platformio-setup)
2. [Arduino IDE Setup (Alternative)](#arduino-ide-setup-alternative)
3. [Pin Configuration Details](#pin-configuration-details)
4. [I²S Audio Interference Troubleshooting](#i²s-audio-interference-troubleshooting)
5. [Display Configuration](#display-configuration)
6. [Touch Calibration](#touch-calibration)
7. [Advanced Troubleshooting](#advanced-troubleshooting)

## Detailed PlatformIO Setup

### Installation Steps

1. **Install Visual Studio Code**:
   - Download from: https://code.visualstudio.com/
   - Install and launch VSCode

2. **Install PlatformIO Extension**:
   - Click Extensions icon (or `Ctrl+Shift+X`)
   - Search for "PlatformIO IDE"
   - Click Install
   - Restart VSCode when prompted

3. **Open Project**:
   - File → Open Folder → Select `cyd_synth` directory
   - PlatformIO will automatically detect `platformio.ini`

### Building and Uploading

**Method 1: PlatformIO Toolbar**
- Build: Click checkmark (✓) in bottom status bar or `Ctrl+Alt+B`
- Upload: Click arrow (→) or `Ctrl+Alt+U`
- Monitor: Click plug (🔌) or `Ctrl+Alt+S`

**Method 2: Terminal Commands**
```bash
pio run                    # Build
pio run --target upload    # Upload
pio device monitor        # Serial monitor
pio run --target clean    # Clean build
```

**Method 3: Command Palette**
- Press `Ctrl+Shift+P`
- Type "PlatformIO" to see available commands

### Serial Monitor

- **Baud Rate**: 115200 (configured in `platformio.ini`)
- **Note**: Code uses Serial2 (not Serial) because IO1 is repurposed for I2S
- Serial2 uses IO17 (TX) and IO16 (RX) by default
- PlatformIO should auto-detect Serial2

### Expected Serial Output

```
CYD Synthesizer Test Starting...
Note: Using Serial2 for debug output (IO17/IO16)
IO1 (P1) is now used for I2S BCK
Initializing display...
Initializing touch screen...
Touch pins: CS=33, IRQ=36, MOSI=32, MISO=39, CLK=25
Touch screen initialized successfully
Initializing I²S for PCM5102...
I²S initialized and started successfully
I2S Config: Sample Rate=44100 Hz, Bits=16, Format=I2S_STAND, APLL=ON
I2S Pins: BCK=1 (P1), LRCK=27 (CN1), DATA=22 (P3/CN1)
DMA: 8 buffers of 128 samples each
Audio task created on Core 0
Setup complete!
```

## Arduino IDE Setup (Alternative)

### Prerequisites

1. **Arduino IDE** (1.8.19+ or 2.x)
2. **ESP32 Board Support**
   - File → Preferences → Additional Board Manager URLs
   - Add: `https://dl.espressif.com/dl/package_esp32_index.json`
   - Tools → Board → Boards Manager → Search "esp32" → Install

### Board Configuration

- **Board**: ESP32 Dev Module
- **Upload Speed**: 115200
- **CPU Frequency**: 240MHz (WiFi/BT)
- **Flash Frequency**: 80MHz
- **Flash Size**: 4MB
- **Partition Scheme**: Default 4MB with spiffs

### Library Installation

1. **TFT_eSPI** by Bodmer
   - Sketch → Include Library → Manage Libraries
   - Search "TFT_eSPI" → Install
   - Configure `User_Setup.h` in library folder

2. **XPT2046_Touchscreen** by Paul Stoffregen
   - Sketch → Include Library → Manage Libraries
   - Search "XPT2046_Touchscreen" → Install

### TFT_eSPI Configuration (Arduino IDE)

After installing TFT_eSPI, edit the library's `User_Setup.h`:
- Navigate to Arduino libraries folder
- Open `TFT_eSPI/User_Setup.h`
- Uncomment your display driver (ILI9341 or ST7789)
- Set pins to match your CYD board

### Upload Process

1. Connect board via USB
2. Select COM port: Tools → Port
3. Click Upload (arrow icon)
4. Monitor: Tools → Serial Monitor (115200 baud)

## Pin Configuration Details

### I²S Pin Assignment Rationale

**Why IO1 for BCK?**
- IO21 conflicts with TFT backlight
- IO26 (speaker connector) cannot be used as GPIO
- IO1 (TX) is available on P1 connector
- Serial switched to Serial2 to free IO1

**Why IO27 for LRCK?**
- IO35 is INPUT ONLY - cannot be used for I2S output
- IO27 is available on CN1 connector
- No conflicts with display/touch

**Why IO22 for DIN?**
- Available on both P3 and CN1 connectors
- No conflicts with other peripherals
- Standard GPIO pin

### Display Pins (CYD Board)

Common configuration for ESP32-2432S028R:
- **TFT_MOSI**: GPIO 23
- **TFT_SCLK**: GPIO 18
- **TFT_CS**: GPIO 15
- **TFT_DC**: GPIO 2
- **TFT_RST**: GPIO 4 (or -1 if connected to board RST)
- **TFT_BL**: GPIO 14 (backlight, optional)

**Note**: Verify these match your specific CYD board variant.

### Touch Pins (XPT2046)

Configured in `src/main.cpp`:
- **XPT2046_CS**: GPIO 33
- **XPT2046_IRQ**: GPIO 36
- **XPT2046_CLK**: GPIO 25
- **XPT2046_MOSI**: GPIO 32
- **XPT2046_MISO**: GPIO 39

Touchscreen uses separate SPI bus (VSPI) from display.

### Pin Conflict Reference

**Cannot Use:**
- IO35: INPUT ONLY - cannot be used for I2S output
- IO21: TFT backlight - conflicts with I2S
- Speaker connector P4: Cannot be used as GPIO

**Available on JST Connectors:**
- **P1**: IO1 (TX), IO3 (RX), VIN, GND
- **CN1**: IO22, IO27, 3.3V, GND
- **P3**: IO22, IO35 (INPUT), IO21 (TFT backlight), GND

## I²S Audio Interference Troubleshooting

### Common Issues

**1. Pin Conflicts**
- Problem: GPIO conflicts with PSRAM, SPI, or display
- Solution: Use pins on JST connectors (IO1, IO22, IO27)
- Reference: Avoid GPIO26/GPIO27 if using PSRAM

**2. Power Supply Noise**
- Problem: Audible artifacts from power supply
- Solutions:
  - Use stable 5V/2A adapter
  - Add decoupling capacitors near DAC
  - Use separate power supply for DAC
  - Ensure proper ground connections

**3. Wi-Fi Interference**
- Problem: Wi-Fi operations cause audible noise
- Solutions:
  - Disable Wi-Fi during audio playback
  - Reduce Wi-Fi transmission power
  - Shield audio components
  - Use wired connections

**4. I²S Configuration Errors**
- Problem: Distorted audio, crackling, or no output
- Solutions:
  - Verify sample rate (44.1kHz or 48kHz)
  - Check bit depth (16-bit)
  - Verify I²S format (I2S_COMM_FORMAT_STAND_I2S)
  - Ensure continuous data stream (no delays in audio task)

**5. Ground Loops**
- Problem: Hum or buzz in audio
- Solutions:
  - Single ground point connection
  - Proper grounding techniques
  - Ground loop isolators if needed

**6. Software Framework Limitations**
- Problem: Audio issues with simultaneous operations
- Solutions:
  - Use FreeRTOS tasks (audio on Core 0, UI on Core 1)
  - High priority for audio task
  - Use DMA for I²S
  - Optimize display updates

### Testing Recommendations

1. Test audio output independently first
2. Test touch/display functionality independently
3. Test audio + display simultaneously
4. Test audio + touch simultaneously
5. Test all three together
6. Monitor for interference or issues

## Display Configuration

### Driver Selection

Edit `include/User_Setup.h` to select display driver:

**For ILI9341:**
```cpp
#define ILI9341_DRIVER
// or
#define ILI9341_2_DRIVER
```

**For ST7789:**
```cpp
#define ST7789_DRIVER
```

### Pin Configuration

Edit `include/User_Setup.h`:
```cpp
#define TFT_MOSI  23
#define TFT_SCLK  18
#define TFT_CS    15
#define TFT_DC    2
#define TFT_RST   4
#define TFT_BL    14  // Backlight (optional)
```

### Display Rotation

In `src/main.cpp`, adjust rotation:
```cpp
tft.setRotation(1);  // 0-3, try different values if display is upside down
```

### Display Dimensions

Configured in `include/User_Setup.h`:
```cpp
#define TFT_WIDTH  320
#define TFT_HEIGHT 240
```

## Touch Calibration

### Coordinate Mapping

Touch coordinates are mapped in `src/main.cpp`:
```cpp
touchX = map(p.x, 200, 3700, 1, tft.width());
touchY = map(p.y, 240, 3800, 1, tft.height());
```

### Calibration Values

These values work for most CYD boards:
- X: 200-3700 → 1-320 (display width)
- Y: 240-3800 → 1-240 (display height)

### Adjusting Calibration

If touch doesn't align with display:
1. Print raw touch values to Serial2
2. Touch corners of display
3. Note min/max X and Y values
4. Adjust mapping in `src/main.cpp`

### Touch Rotation

Match touch rotation to display rotation:
```cpp
ts.setRotation(1);  // Should match tft.setRotation()
```

## Advanced Troubleshooting

### Board Not Recognized

**Windows:**
- Check Device Manager for COM port
- Install CP2102 or CH340 drivers if needed
- Try different USB port (prefer USB 2.0)

**Mac/Linux:**
- Check `/dev/tty*` or `/dev/cu*` for device
- Usually works without additional drivers
- May need to add user to dialout group (Linux)

### Upload Fails

1. **Hold BOOT Button**: Some boards require this during upload
2. **Lower Upload Speed**: Try 921600, 460800, or 115200
3. **Manual Reset**: Press RESET, then immediately click Upload
4. **Check Port**: Verify correct COM port selected
5. **Check Cable**: Use data-capable USB cable

### Build Errors

**Missing Libraries:**
- PlatformIO should auto-install
- If not: `pio lib install`
- Check `platformio.ini` for correct library names

**Display Configuration:**
- Verify driver in `include/User_Setup.h`
- Check pin assignments match your board
- Ensure `extra_scripts.py` is copying `User_Setup.h`

**Compilation Errors:**
- Check syntax in `src/main.cpp`
- Verify all includes are correct
- Check PlatformIO output for specific errors

### No Audio Output

**Wiring Checklist:**
- [ ] IO1 (YellowBlack) → PCM5102 BCK
- [ ] IO22 (Blue) → PCM5102 DIN
- [ ] IO27 (Yellow) → PCM5102 LCK
- [ ] GND (Black) → PCM5102 GND
- [ ] 3.3V (Red) → PCM5102 VIN

**Configuration Checks:**
- [ ] I²S initialized successfully (check Serial2)
- [ ] Audio task created (check Serial2)
- [ ] Sample rate: 44100 Hz
- [ ] Bit depth: 16-bit
- [ ] Format: I2S_STAND
- [ ] APLL enabled (if needed)

**Hardware Checks:**
- [ ] DAC powered (3.3V)
- [ ] Ground connected
- [ ] Audio output connected (speaker/headphones)
- [ ] Volume level adequate

### Display Not Working

**Configuration:**
- Verify driver type (ILI9341 or ST7789)
- Check pin assignments in `include/User_Setup.h`
- Try different rotation values (0-3)

**Hardware:**
- Check SPI connections
- Verify backlight pin (if applicable)
- Ensure power is connected

### Touch Not Working

**Configuration:**
- Verify touch pins (CS=33, IRQ=36)
- Check touch SPI pins (CLK=25, MOSI=32, MISO=39)
- Match touch rotation to display rotation

**Calibration:**
- Adjust coordinate mapping if needed
- Check raw touch values in Serial2
- Verify touch controller type (XPT2046)

### Serial Monitor Not Working

**Serial2 Configuration:**
- Code uses Serial2 (not Serial)
- Serial2 uses IO17 (TX) and IO16 (RX)
- PlatformIO should auto-detect

**If Not Working:**
- Check COM port selection
- Verify baud rate: 115200
- Try different USB port
- Check USB cable (data-capable)

## Additional Resources

- **PlatformIO Documentation**: https://docs.platformio.org/
- **ESP32 Platform**: https://docs.platformio.org/en/latest/platforms/espressif32.html
- **TFT_eSPI Library**: https://github.com/Bodmer/TFT_eSPI
- **XPT2046 Library**: https://github.com/PaulStoffregen/XPT2046_Touchscreen
- **ESP32 I²S API**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html

## Code Reference

### Key File Locations

- **I²S Pin Definitions**: `src/main.cpp` lines 52-55
- **Touch Pin Definitions**: `src/main.cpp` lines 31-35
- **Display Configuration**: `include/User_Setup.h`
- **I²S Configuration**: `src/main.cpp` lines 166-183
- **Audio Task**: `src/main.cpp` lines 79-122
- **Touch Handling**: `src/main.cpp` lines 251-280

### Configuration Constants

**Audio:**
- `SAMPLE_RATE`: 44100 Hz
- `SINE_FREQ`: 440.0 Hz (A4 note)
- `AMPLITUDE`: 16000 (16-bit, with headroom)

**I²S:**
- `I2S_BCK_PIN`: 1 (P1 connector)
- `I2S_LRCK_PIN`: 27 (CN1 connector)
- `I2S_DATA_PIN`: 22 (CN1 connector)

**Touch:**
- `XPT2046_CS`: 33
- `XPT2046_IRQ`: 36
- `XPT2046_CLK`: 25
- `XPT2046_MOSI`: 32
- `XPT2046_MISO`: 39

