# CYD Synthesizer

ESP32-based synthesizer using the Cheap Yellow Display (CYD) board with PCM5102 DAC. The idea here is to try to make something that makes pretty sounds and uses the limitations of the touchscreen as a feature rather than an annoyance. I've not quite managed it, but it's getting there. 

Currently, Dual oscillators (Base sine, higher saw, tweakable by sliders) go through a hardcoded ASR, and a single fixed Q biquad filter, with the cutoff controlled by the vertical part of the top of the screen. The horizontal part is pentatonic (for now, more options later). The delay has a stereo offset (fixed for now) and an LFO for varying the time. The result is a not-terrible-sounding delay synth.

DEMO VIDEO WILL GO HERE WHEN I CAN RECORD IT

Up next is implementing envelopes for the filters and oscillators, and then deciding what should be user-facing and what should be hidden away. I can imagine a future where there's a simple version of this synth, and a more involved version with some menu diving and the possibility of choosing what controls you have on the screen, or perhaps even presets via sd card. The slot is there after all!

## Quick Start

### Prerequisites

1. Visual Studio Code with PlatformIO IDE extension
2. USB cable to connect the CYD board
3. PCM5102 DAC wired as described in the Pinout section below

### Installation

1. Install PlatformIO:
   - Open VSCode
   - Install "PlatformIO IDE" extension
   - Restart VSCode

2. Open Project:
   - File → Open Folder → Select `cyd_synth` directory

3. Build and Upload:
   - Click the checkmark icon in bottom status bar to build
   - Click the arrow icon to upload
   - Click the plug icon to open serial monitor (115200 baud)

### First Run

After uploading, you should see:
- Display showing pentatonic scale zones in the top half
- Parameter controls in the bottom half
- Touch screen control:
  - Top half X-axis: Controls pitch (quantized to pentatonic scale)
  - Top half Y-axis: Controls low-pass filter cutoff
  - Bottom half: Parameter sliders for delay, LFO, base tone, and upper tone

Note: Serial debugging uses Serial2 (not Serial) because IO1 is repurposed for I2S. This probably isn't something you'll run in to though, USB serial works as normal and the serial monitor should work automatically.

## Pinout

### I2S to PCM5102 DAC Wiring

All connections use JST connectors P1 and CN1 (non-invasive setup):

CN1 Connector (JST1):
| ESP32 Pin | Wire Color | PCM5102 Pin | Signal |
|-----------|------------|-------------|--------|
| GND       | Black      | GND        | Ground |
| IO22      | Blue       | DIN        | Data Input |
| IO27      | Yellow     | LCK        | Left/Right Clock |
| 3.3V      | Red        | VIN        | Power |

P1 Connector (JST2):
| ESP32 Pin | Wire Color | PCM5102 Pin | Signal |
|-----------|------------|-------------|--------|
| IO1 (TX)  | YellowBlack| BCK        | Bit Clock |

Important Notes:
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

Configured in `src/config.h`:
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
│   ├── main.cpp        # Main application code
│   ├── config.h        # Centralized configuration
│   ├── hardware/       # Hardware abstraction modules
│   ├── audio/          # Audio processing modules
│   └── ui/             # User interface modules
├── include/
│   └── User_Setup.h     # TFT_eSPI display configuration
└── extra_scripts.py    # Auto-copies User_Setup.h to TFT_eSPI library
```

### Key Settings (platformio.ini)

- Platform: espressif32
- Board: esp32dev
- Framework: arduino
- Serial Monitor: 115200 baud
- Libraries: 
  - TFT_eSPI@^2.5.43
  - XPT2046_Touchscreen

  *Note*: That I had to put the full github address of the touchscreen library into my platformio.ini before it could find the library, i.e:

  ```bash
  lib_deps = 
    bodmer/TFT_eSPI@^2.5.43
    https://github.com/PaulStoffregen/XPT2046_Touchscreen.git
  ```

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

## Features

### Audio Generation
- Dual oscillator: Sine wave + Sawtooth wave
- Pentatonic scale quantization (5 notes per octave)
- ADSR envelope with smooth attack and release
- Low-pass filter with resonance (Q control)
- Stereo delay effect with LFO modulation
- Variable sawtooth interval (5, 7, 9, 12, or 14 semitones above sine)

### Controls

Top Half of Screen:
- X-axis: Pitch control (quantized to pentatonic scale, 80Hz to 2kHz)
- Y-axis: Low-pass filter cutoff (40Hz to 4kHz)

Bottom Half of Screen (5 Parameter Sliders):
- Delay Time: 50ms to 1000ms (logarithmic)
- LFO Depth: 0ms to 100ms (modulates delay time)
- LFO Speed: 0.1Hz to 50Hz (logarithmic)
- Base Tone: 5 detents (whole tone steps, shifts entire scale)
- Upper Tone: 5 detents (sawtooth interval: 5, 7, 9, 12, 14 semitones)

### Technical Details
- Sample Rate: 44100 Hz
- Audio Processing: Core 0 (FreeRTOS task)
- UI Processing: Core 1 (main loop)
- Delay Sample Rate: 22050 Hz (configurable, saves memory)
- Thread-safe: Volatile variables for inter-core communication

## Troubleshooting

### No Audio Output
- Verify wiring matches pinout above (especially IO1, IO22, IO27)
- Check power connections (3.3V Red wire and GND Black wire on CN1)
- Ensure PCM5102 VOUT is connected to amplifier/headphones
- Check serial monitor for I2S initialization errors

### Display Not Working
- Verify display driver in `include/User_Setup.h` (ILI9341_DRIVER or ILI9341_2_DRIVER)
- Check pin assignments match your CYD board
- Try different rotation values (0-3) in `src/main.cpp`

### Touch Not Working
- Verify touch pins in `src/config.h` (CS=33, IRQ=36)
- Check that touchscreen SPI pins are correct (CLK=25, MOSI=32, MISO=39)
- Touch coordinates may need calibration - adjust mapping in `src/hardware/touch.cpp`

### Serial Monitor Not Working
- Code uses Serial2 (not Serial) because IO1 is repurposed for I2S
- Serial2 uses IO17 (TX) and IO16 (RX) by default
- PlatformIO should auto-detect - if not, check COM port settings

### Build Errors
- See `ERROR_LOG.md` for common build errors and solutions
- Ensure `platformio.ini` does NOT have `-DUSER_SETUP_LOADED=1` flag
- Check that `extra_scripts.py` is running (should see it in build output)

## Code Locations

- Configuration: `src/config.h`
- I2S Pin Definitions: `src/config.h`
- Touch Pin Definitions: `src/config.h`
- Display Configuration: `include/User_Setup.h`
- Main Application: `src/main.cpp`
- Architecture Details: `ARCHITECTURE.md`

## Additional Information

For detailed architecture, signal flow, and module descriptions, see `ARCHITECTURE.md`.

For troubleshooting specific errors, see `ERROR_LOG.md`.

## Libraries

- TFT_eSPI: Display library (configured via `include/User_Setup.h`)
- XPT2046_Touchscreen: Touch input library
- ESP32 I2S: Built-in ESP32 I2S driver

## Credits

- Brian Lough basically triggered my interest in this device, and [his CYD github repo](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display) was the genesis of this project.
- [TFT_eSPI: Bodmer](https://github.com/Bodmer/TFT_eSPI)
- [XPT2046_Touchscreen: Paul Stoffregen](https://github.com/PaulStoffregen/XPT2046_Touchscreen)
