# PlatformIO Setup and Flashing Instructions

## Prerequisites

1. **Visual Studio Code** (VSCode)
   - Download from: https://code.visualstudio.com/

2. **PlatformIO IDE Extension**
   - Install from VSCode Extensions marketplace
   - Search for "PlatformIO IDE" by PlatformIO
   - Or install via command palette: `Ctrl+Shift+P` → "Extensions: Install Extensions"

3. **USB Cable**
   - Use a data-capable USB cable (not charge-only)
   - Connect to the USB port on the CYD board

## Initial Setup

### 1. Install PlatformIO Extension

1. Open VSCode
2. Click the Extensions icon (or press `Ctrl+Shift+X`)
3. Search for "PlatformIO IDE"
4. Click "Install"
5. Wait for installation to complete
6. **Restart VSCode** when prompted

### 2. Open the Project

1. In VSCode, go to **File** → **Open Folder**
2. Select the `cyd_synth` directory
3. PlatformIO should automatically detect the `platformio.ini` file

### 3. Configure Display Pins (IMPORTANT)

The display pins are configured in `include/User_Setup.h`. You may need to adjust these based on your specific CYD board:

1. Open `include/User_Setup.h`
2. Adjust the display pins if needed:
   ```cpp
   #define TFT_MOSI  23
   #define TFT_SCLK  18
   #define TFT_CS    15
   #define TFT_DC    2
   #define TFT_RST   4
   #define TFT_BL    14  // Backlight (optional)
   ```
3. If your board uses ST7789 instead of ILI9341, change:
   ```cpp
   #define ILI9341_DRIVER
   ```
   to:
   ```cpp
   #define ST7789_DRIVER
   ```

**Note**: The `extra_scripts.py` file automatically copies `User_Setup.h` to the TFT_eSPI library directory during build, so you don't need to manually copy it.

### 4. Install Dependencies

PlatformIO will automatically download libraries when you build, but you can manually install them:

1. Click the PlatformIO icon in the left sidebar (ant icon)
2. Go to **Project Tasks** → **esp32dev** → **General**
3. Click **"Build"** - this will download all dependencies
4. Or use the terminal: `pio lib install`

## Building and Uploading

### Method 1: Using PlatformIO Toolbar

1. **Build the Project**:
   - Click the checkmark icon (✓) in the bottom status bar
   - Or press `Ctrl+Alt+B`
   - Or use PlatformIO sidebar → **Project Tasks** → **esp32dev** → **General** → **Build**

2. **Upload to Board**:
   - Click the right arrow icon (→) in the bottom status bar
   - Or press `Ctrl+Alt+U`
   - Or use PlatformIO sidebar → **Project Tasks** → **esp32dev** → **General** → **Upload**

3. **Upload and Monitor**:
   - Click the plug icon (🔌) in the bottom status bar
   - Or press `Ctrl+Alt+S`
   - This uploads and opens the serial monitor

### Method 2: Using Terminal

1. Open VSCode integrated terminal (`Ctrl+` ` or View → Terminal)

2. **Build**:
   ```bash
   pio run
   ```

3. **Upload**:
   ```bash
   pio run --target upload
   ```

4. **Monitor Serial Output**:
   ```bash
   pio device monitor
   ```

5. **Upload and Monitor** (combined):
   ```bash
   pio run --target upload && pio device monitor
   ```

### Method 3: Using Command Palette

1. Press `Ctrl+Shift+P` to open command palette
2. Type "PlatformIO" to see available commands:
   - `PlatformIO: Build`
   - `PlatformIO: Upload`
   - `PlatformIO: Clean`
   - `PlatformIO: Serial Monitor`

## Serial Monitor

### Opening Serial Monitor

1. Click the plug icon (🔌) in the bottom status bar
2. Or press `Ctrl+Alt+S`
3. Or use PlatformIO sidebar → **Project Tasks** → **esp32dev** → **General** → **Monitor**
4. Or use terminal: `pio device monitor`

### Serial Monitor Settings

The serial monitor is configured in `platformio.ini`:
- **Baud Rate**: 115200 (set via `monitor_speed = 115200`)
- **Filters**: Default filters are applied
- **Auto-scroll**: Enabled by default

### Expected Output

You should see:
```
CYD Synthesizer Test Starting...
Initializing display...
Initializing touch screen...
Initializing I²S...
I²S initialized successfully
Audio task created on Core 0
Setup complete!
Touch detected: X=120 Y=160
...
```

## Troubleshooting

### Board Not Recognized

1. **Check USB Connection**:
   - Ensure USB cable is data-capable
   - Try different USB port (prefer USB 2.0)
   - Check Device Manager (Windows) or `ls /dev/tty*` (Linux/Mac)

2. **Check COM Port**:
   - PlatformIO should auto-detect the port
   - If not, check `platformio.ini` for `upload_port` setting
   - Or use `pio device list` to see available ports

3. **Install USB Drivers**:
   - Windows: CP2102 or CH340 drivers
   - Mac/Linux: Usually works without drivers

### Upload Fails

1. **Hold BOOT Button**:
   - Some boards require holding BOOT during upload
   - Try holding BOOT, then click Upload

2. **Lower Upload Speed**:
   - Edit `platformio.ini`
   - Change `upload_speed = 115200` to a lower value (e.g., `921600`, `460800`)

3. **Manual Reset**:
   - Press RESET button on board
   - Immediately click Upload in VSCode

4. **Check Port**:
   - Ensure correct port is selected
   - Use `pio device list` to verify

### Build Errors

1. **Missing Libraries**:
   - PlatformIO should auto-install libraries
   - If not, run `pio lib install` in terminal
   - Check `platformio.ini` for correct library names

2. **Display Configuration**:
   - Verify display driver in `build_flags` (ILI9341 or ST7789)
   - Check pin assignments match your board
   - See `PIN_CONFIGURATION.md` for reference

3. **Compilation Errors**:
   - Check for syntax errors in `src/main.cpp`
   - Verify all includes are correct
   - Check PlatformIO output for specific error messages

### No Audio Output

1. **Check Wiring**:
   - Verify I²S pins (IO21, IO22, IO35)
   - Check power connections (3.3V and GND)
   - Verify DAC is powered

2. **Check Serial Output**:
   - Look for I²S initialization errors
   - Verify "I²S initialized successfully" message

3. **Check Audio Output**:
   - Ensure speaker/headphones connected to 3.5mm jack
   - Check volume level
   - Verify DAC is working

### Display Not Working

1. **Check Display Configuration**:
   - Verify display driver in `platformio.ini`
   - Check pin assignments
   - Try different rotation values (0-3)

2. **Check Build Flags**:
   - Ensure `USER_SETUP_LOADED=1` is set
   - Verify display driver flag (ILI9341_DRIVER or ST7789_DRIVER)
   - Check pin definitions are correct

3. **Check Connections**:
   - Verify SPI connections
   - Check backlight pin if applicable
   - Ensure power is connected

### Touch Not Working

1. **Check Touch Pins**:
   - Verify `TOUCH_CS` and `TOUCH_IRQ` in `src/main.cpp`
   - Check touch controller type (XPT2046)
   - Verify SPI connections

2. **Check Touch Library**:
   - Ensure XPT2046_Touchscreen library is installed
   - Check library version compatibility

3. **Check Calibration**:
   - Touch coordinates may need adjustment
   - Try different mapping in `src/main.cpp`
   - Check touch rotation matches display rotation

## PlatformIO vs Arduino IDE

### Advantages of PlatformIO

- **Better Code Management**: Proper project structure with `src/`, `include/`, `lib/`
- **Library Management**: Automatic dependency resolution
- **Better Debugging**: Integrated debugging support
- **Multiple Environments**: Easy to support multiple board configurations
- **Version Control**: Better `.gitignore` support
- **CI/CD**: Can be integrated into build pipelines
- **Code Completion**: Better IntelliSense support in VSCode

### Key Differences

- **File Structure**: `src/main.cpp` instead of `.ino` files
- **Library Management**: Libraries defined in `platformio.ini` instead of Library Manager
- **Configuration**: `platformio.ini` instead of board selection in IDE
- **Build System**: PlatformIO handles compilation automatically

## Quick Reference

### Common Commands

```bash
# Build project
pio run

# Upload to board
pio run --target upload

# Clean build files
pio run --target clean

# Serial monitor
pio device monitor

# List devices
pio device list

# Install libraries
pio lib install

# Update libraries
pio lib update
```

### PlatformIO.ini Settings

| Setting | Value | Description |
|---------|-------|-------------|
| `platform` | `espressif32` | ESP32 platform |
| `board` | `esp32dev` | Board type |
| `framework` | `arduino` | Arduino framework |
| `monitor_speed` | `115200` | Serial monitor baud rate |
| `upload_speed` | `115200` | Upload speed |

## Additional Resources

- **PlatformIO Documentation**: https://docs.platformio.org/
- **ESP32 Platform**: https://docs.platformio.org/en/latest/platforms/espressif32.html
- **TFT_eSPI Library**: https://github.com/Bodmer/TFT_eSPI
- **XPT2046 Library**: https://github.com/PaulStoffregen/XPT2046_Touchscreen

## Next Steps

1. Build and upload the test sketch
2. Verify audio output (440Hz sine wave)
3. Test display functionality
4. Test touch input
5. Adjust pin configurations if needed
6. See `PIN_CONFIGURATION.md` for pin reference
7. See `I2S_AUDIO_INTERFERENCE_NOTES.md` for troubleshooting audio issues

