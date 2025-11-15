# CYD Synthesizer

ESP32-based synthesizer using the Cheap Yellow Display (CYD) board with PCM5102 DAC.

## Quick Start

### Prerequisites

1. **Visual Studio Code** with **PlatformIO IDE** extension
2. USB cable to connect the CYD board
3. PCM5102 DAC wired as described in Pinout section below

### Installation

1. **Install PlatformIO**:
   - Open VSCode
   - Install "PlatformIO IDE" extension
   - Restart VSCode

2. **Open Project**:
   - File → Open Folder → Select `cyd_synth` directory

3. **Build and Upload**:
   - Click the checkmark (✓) in bottom status bar to build
   - Click the arrow (→) to upload
   - Click the plug (🔌) to open serial monitor (115200 baud)

### First Run

After uploading, you should see:
- Display showing "CYD Synthesizer" and status information
- 440Hz sine wave audio output (A4 note)
- Touch screen responding to touches with visual feedback

**Note**: Serial debugging uses Serial2 (not Serial) because IO1 is repurposed for I2S. The serial monitor should work automatically.

## Pinout

### I²S to PCM5102 DAC Wiring

All connections use JST connectors P1 and CN1 (non-invasive setup):

**CN1 Connector (JST1):**
| ESP32 Pin | Wire Color | PCM5102 Pin | Signal |
|-----------|------------|------------|--------|
| GND       | Black      | GND        | Ground |
| IO22      | Blue       | DIN        | Data Input |
| IO27      | Yellow     | LCK        | Left/Right Clock |
| 3.3V      | Red        | VIN        | Power |

**P1 Connector (JST2):**
| ESP32 Pin | Wire Color | PCM5102 Pin | Signal |
|-----------|------------|------------|--------|
| IO1 (TX)  | YellowBlack| BCK        | Bit Clock |

**Important Notes:**
- IO35 is INPUT ONLY - cannot be used for I2S output
- IO21 is used for TFT backlight - conflicts with I2S
- Speaker connector P4 cannot be used as GPIO
- IO1 (TX) is repurposed from UART - Serial uses Serial2 instead (IO17/IO16)

### Display Pins

Configured in `include/User_Setup.h`:
- TFT_MOSI: GPIO 23
- TFT_SCLK: GPIO 18
- TFT_CS: GPIO 15
- TFT_DC: GPIO 2
- TFT_RST: GPIO 4 (or -1 if connected to board RST)
- TFT_BL: GPIO 14 (backlight, optional)

### Touch Pins

Configured in `src/main.cpp`:
- XPT2046_CS: GPIO 33
- XPT2046_IRQ: GPIO 36
- XPT2046_CLK: GPIO 25
- XPT2046_MOSI: GPIO 32
- XPT2046_MISO: GPIO 39

## PlatformIO Configuration

### Project Structure

```
cyd_synth/
├── platformio.ini      # PlatformIO configuration
├── src/
│   └── main.cpp        # Main application code
├── include/
│   └── User_Setup.h    # TFT_eSPI display configuration
└── extra_scripts.py    # Auto-copies User_Setup.h to TFT_eSPI library
```

### Key Settings (platformio.ini)

- **Platform**: espressif32
- **Board**: esp32dev
- **Framework**: arduino
- **Serial Monitor**: 115200 baud
- **Libraries**: 
  - TFT_eSPI@^2.5.43
  - XPT2046_Touchscreen

### Display Configuration

The `extra_scripts.py` automatically copies `include/User_Setup.h` to the TFT_eSPI library during build. Edit `include/User_Setup.h` to adjust:
- Display driver (ILI9341 or ST7789)
- Pin assignments
- SPI frequency
- Display settings

### Common PlatformIO Commands

```bash
# Build project
pio run

# Upload to board
pio run --target upload

# Serial monitor
pio device monitor

# Clean build files
pio run --target clean

# List connected devices
pio device list
```

## Troubleshooting

### No Audio Output
- Verify wiring matches pinout above (especially IO1, IO22, IO27)
- Check power connections (3.3V Red wire and GND Black wire on CN1)
- Ensure PCM5102 VOUT is connected to amplifier/headphones
- Check serial monitor for I²S initialization errors

### Display Not Working
- Verify display driver in `include/User_Setup.h` (ILI9341_DRIVER or ILI9341_2_DRIVER)
- Check pin assignments match your CYD board
- Try different rotation values (0-3) in `src/main.cpp`

### Touch Not Working
- Verify touch pins in `src/main.cpp` (CS=33, IRQ=36)
- Check that touchscreen SPI pins are correct (CLK=25, MOSI=32, MISO=39)
- Touch coordinates may need calibration - adjust mapping in `src/main.cpp`

### Serial Monitor Not Working
- Code uses Serial2 (not Serial) because IO1 is repurposed for I2S
- Serial2 uses IO17 (TX) and IO16 (RX) by default
- PlatformIO should auto-detect - if not, check COM port settings

## Code Locations

- **I²S Pin Definitions**: `src/main.cpp` lines 52-55
- **Touch Pin Definitions**: `src/main.cpp` lines 31-35
- **Display Configuration**: `include/User_Setup.h`
- **I²S Configuration**: `src/main.cpp` lines 166-183

## Features

- **440Hz Sine Wave**: Test audio output via I²S to PCM5102 DAC
- **Display Status**: Shows status information on TFT screen
- **Touch Input**: Tests touchscreen functionality with visual feedback
- **FreeRTOS Tasks**: Audio processing on Core 0, UI on Core 1

## Additional Information

For detailed troubleshooting, advanced configuration, and reference information, see `other_instructions.md`.

## Libraries

- **TFT_eSPI**: Display library (configured via `include/User_Setup.h`)
- **XPT2046_Touchscreen**: Touch input library
- **ESP32 I²S**: Built-in ESP32 I²S driver

## Credits

- ESP32 Platform: Espressif Systems
- TFT_eSPI: Bodmer
- XPT2046_Touchscreen: Paul Stoffregen
