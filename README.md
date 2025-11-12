# CYD Synthesizer

ESP32-based synthesizer using the Cheap Yellow Display (CYD) board with PCM1502 DAC.

## Hardware

- **Board**: ESP32-2432S028R (Cheap Yellow Display)
- **DAC**: PCM1502
- **I²S Connections**:
  - IO21 → BCK (Bit Clock)
  - IO22 → DIN (Data Input)
  - IO35 → LCK (Left/Right Clock / Word Select)
  - GND → GND
  - 3.3V → VIN (separate power supply, grounds joined)

## Project Structure

```
cyd_synth/
├── platformio.ini          # PlatformIO configuration
├── src/
│   └── main.cpp            # Main application code
├── include/
│   └── User_Setup.h        # TFT_eSPI display configuration
├── .gitignore              # Git ignore file
├── README.md               # This file
├── PLATFORMIO_SETUP.md     # PlatformIO setup and flashing instructions
├── PIN_CONFIGURATION.md    # Pin configuration reference
└── I2S_AUDIO_INTERFERENCE_NOTES.md  # Audio interference troubleshooting
```

## Quick Start

### Prerequisites

1. **Visual Studio Code** with **PlatformIO IDE** extension
2. USB cable to connect the CYD board
3. PCM1502 DAC wired as described above

### Setup

1. **Install PlatformIO**:
   - Open VSCode
   - Install "PlatformIO IDE" extension
   - Restart VSCode

2. **Configure Display**:
   - Edit `include/User_Setup.h` to match your CYD board's display pins
   - Adjust driver type (ILI9341 or ST7789) if needed
   - See `PIN_CONFIGURATION.md` for reference

3. **Build and Upload**:
   - Click the checkmark (✓) to build
   - Click the arrow (→) to upload
   - Click the plug (🔌) to open serial monitor

### Detailed Instructions

See `PLATFORMIO_SETUP.md` for comprehensive setup and flashing instructions.

## Current Features

- **440Hz Sine Wave**: Test audio output via I²S to PCM1502 DAC
- **Display Status**: Shows status information on TFT screen
- **Touch Input**: Tests touchscreen functionality with visual feedback
- **FreeRTOS Tasks**: Audio processing on Core 0, UI on Core 1

## Configuration

### Display Pins

Edit `include/User_Setup.h` to configure display pins. Common CYD board pins:
- TFT_MOSI: 23
- TFT_SCLK: 18
- TFT_CS: 15
- TFT_DC: 2
- TFT_RST: 4
- TFT_BL: 14 (backlight, optional)

### Touch Pins

Edit `src/main.cpp` to configure touch pins:
- TOUCH_CS: 5
- TOUCH_IRQ: 25 (optional)

### I²S Pins

Currently configured in `src/main.cpp`:
- I2S_BCK_PIN: 21
- I2S_DATA_PIN: 22
- I2S_LRCK_PIN: 35

## Troubleshooting

- **Display not working**: Check `include/User_Setup.h` pin configuration
- **Touch not working**: Check touch pin assignments in `src/main.cpp`
- **No audio**: Check I²S wiring and power connections
- **Audio interference**: See `I2S_AUDIO_INTERFERENCE_NOTES.md`

## Documentation

- `PLATFORMIO_SETUP.md`: PlatformIO setup and flashing instructions
- `PIN_CONFIGURATION.md`: Pin configuration reference
- `I2S_AUDIO_INTERFERENCE_NOTES.md`: Audio interference troubleshooting guide
- `USB_FLASHING_INSTRUCTIONS.md`: Arduino IDE flashing instructions (legacy)

## Libraries

- **TFT_eSPI**: Display library (configured via `include/User_Setup.h`)
- **XPT2046_Touchscreen**: Touch input library
- **ESP32 I²S**: Built-in ESP32 I²S driver

## Development

This project uses PlatformIO for development. All code is in `src/main.cpp`. The project structure follows PlatformIO conventions:

- `src/`: Source code
- `include/`: Header files (TFT_eSPI configuration)
- `platformio.ini`: Project configuration
- `.pio/`: Build artifacts (gitignored)

## License

[Add your license here]

## Credits

- ESP32 Platform: Espressif Systems
- TFT_eSPI: Bodmer
- XPT2046_Touchscreen: Paul Stoffregen

