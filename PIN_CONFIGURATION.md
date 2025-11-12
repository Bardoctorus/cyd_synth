# Pin Configuration Reference

## Current Wiring Configuration

### I²S to PCM1502 DAC (CONFIRMED)
- **GND** → GND (shared ground)
- **IO35** → LCK (Left/Right Clock / Word Select)
- **IO22** → DIN (Data Input)
- **IO21** → BCK (Bit Clock)
- **3.3V** → VIN (separate power supply, grounds joined)

### Display Pins (NEEDS VERIFICATION)
The CYD board uses different display controllers depending on the version. Common configurations:

**ILI9341 Display (common):**
- TFT_MOSI: GPIO 23
- TFT_SCLK: GPIO 18
- TFT_CS: GPIO 15
- TFT_DC: GPIO 2
- TFT_RST: GPIO 4
- TFT_BL: GPIO 14 (backlight, optional)

**ST7789 Display (alternative):**
- TFT_MOSI: GPIO 23
- TFT_SCLK: GPIO 18
- TFT_CS: GPIO 15
- TFT_DC: GPIO 2
- TFT_RST: GPIO 4
- TFT_BL: GPIO 14 (backlight, optional)

### Touch Controller Pins (NEEDS VERIFICATION)
Most CYD boards use XPT2046 touch controller:

**XPT2046 Touch:**
- TOUCH_CS: GPIO 5
- TOUCH_IRQ: GPIO 25 (optional, can use -1 if not connected)
- SPI_MISO: GPIO 19 (shared with display SPI)
- SPI_MOSI: GPIO 23 (shared with display SPI)
- SPI_SCLK: GPIO 18 (shared with display SPI)

### Other CYD Board Pins
- **MicroSD Card**: Usually on SPI bus (may conflict if used)
- **RGB LED**: Various GPIOs (check board documentation)
- **Buttons**: Various GPIOs (check board documentation)

## Important Notes

1. **Pin Conflicts**: 
   - I²S pins (IO21, IO22, IO35) should not conflict with display/touch
   - Display and touch share SPI bus (MOSI, SCLK) - this is normal
   - Ensure no other peripherals use the same pins

2. **Verification Required**:
   - Display controller type (ILI9341 vs ST7789)
   - Exact pin assignments for your specific CYD board
   - Touch controller type and pins
   - Check board schematic or documentation

3. **TFT_eSPI Configuration**:
   - After installing TFT_eSPI library, edit `User_Setup.h` or `User_Setup_Select.h`
   - Uncomment the correct display driver
   - Set the correct pin numbers
   - Example for ILI9341:
     ```cpp
     #define ILI9341_DRIVER
     #define TFT_MOSI 23
     #define TFT_SCLK 18
     #define TFT_CS   15
     #define TFT_DC   2
     #define TFT_RST  4
     #define TFT_BL   14
     ```

4. **Touch Calibration**:
   - Touch coordinates may need calibration
   - Adjust mapping in the sketch if touch doesn't align with display
   - Some boards have inverted X or Y axis

## How to Find Your Board's Pins

1. **Check Board Documentation**: Look for ESP32-2432S028R schematic or pinout
2. **Check TFT_eSPI Examples**: Look for CYD-specific examples
3. **Check Serial Output**: Some boards print pin configuration on startup
4. **Test with Simple Sketch**: Use a simple display test to verify pins
5. **Check GitHub**: Search for "ESP32-2432S028R" or "Cheap Yellow Display" examples

## Testing Pin Configuration

1. Start with display only (comment out touch and audio)
2. Verify display works with known pin configuration
3. Add touch functionality
4. Add audio last (to isolate any interference issues)

## Current Sketch Configuration

The sketch `cyd_synth_test.ino` uses these assumptions:
- Display: TFT_eSPI library (configure in User_Setup.h)
- Touch: XPT2046_Touchscreen library
- Touch CS: GPIO 5
- Touch IRQ: GPIO 25
- I²S: IO21 (BCK), IO22 (DIN), IO35 (LCK)

**You may need to adjust these in the sketch based on your board!**

