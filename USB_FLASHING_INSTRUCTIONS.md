# USB Flashing Instructions for CYD Synthesizer

## Prerequisites

1. **Arduino IDE** (version 1.8.19 or later, or Arduino IDE 2.x)
   - Download from: https://www.arduino.cc/en/software

2. **USB Cable**
   - Use a data-capable USB cable (not charge-only)
   - Connect to the USB port on the CYD board

3. **USB-to-Serial Driver** (if needed)
   - Most modern operating systems recognize ESP32 boards automatically
   - If your computer doesn't recognize the board, you may need:
     - **Windows**: CP2102 or CH340 drivers (usually auto-installed)
     - **Mac/Linux**: Usually works without additional drivers

## Step-by-Step Flashing Instructions

### 1. Install ESP32 Board Support in Arduino IDE

1. Open Arduino IDE
2. Go to **File** > **Preferences**
3. In the "Additional Board Manager URLs" field, add:
   ```
   https://dl.espressif.com/dl/package_esp32_index.json
   ```
   (If you already have URLs, separate them with commas)
4. Click **OK**

5. Go to **Tools** > **Board** > **Boards Manager**
6. Search for "**esp32**"
7. Install "**esp32 by Espressif Systems**" (latest version)
8. Wait for installation to complete

### 2. Select the Correct Board

1. Go to **Tools** > **Board** > **ESP32 Arduino**
2. Select **"ESP32 Dev Module"** (or your specific ESP32 board if listed)

### 3. Configure Board Settings

1. Go to **Tools** > **Upload Speed** and set to **"115200"** (or lower if you have issues)
2. Go to **Tools** > **CPU Frequency** and select **"240MHz (WiFi/BT)"**
3. Go to **Tools** > **Flash Frequency** and select **"80MHz"**
4. Go to **Tools** > **Flash Size** and select **"4MB (32Mb)"** (or match your board)
5. Go to **Tools** > **Partition Scheme** and select **"Default 4MB with spiffs"**
6. Go to **Tools** > **Core Debug Level** and select **"None"** (or "Info" for debugging)

### 4. Install Required Libraries

1. Go to **Sketch** > **Include Library** > **Manage Libraries**
2. Install the following libraries (search for each):
   - **TFT_eSPI** by Bodmer (for display)
   - **XPT2046_Touchscreen** by Paul Stoffregen (for touch input)

3. **Configure TFT_eSPI** (IMPORTANT):
   - After installing TFT_eSPI, you need to configure it for your display
   - Navigate to your Arduino libraries folder
   - Open `TFT_eSPI/User_Setup.h` or `TFT_eSPI/User_Setup_Select.h`
   - Uncomment the line for your display driver (likely ILI9341 or ST7789)
   - Set the correct pins for your CYD board
   - Common CYD pin configuration:
     ```cpp
     #define TFT_MOSI 23
     #define TFT_SCLK 18
     #define TFT_CS   15
     #define TFT_DC   2
     #define TFT_RST  4
     #define TFT_BL   14  // Backlight (if applicable)
     ```
   - **Note**: Pin numbers may vary - check your CYD board documentation

### 5. Connect the Board

1. Connect the CYD board to your computer via USB cable
2. Wait a few seconds for the computer to recognize the board
3. Check Device Manager (Windows) or `ls /dev/tty*` (Linux/Mac) to see the COM port

### 6. Select the COM Port

1. In Arduino IDE, go to **Tools** > **Port**
2. Select the COM port for your ESP32 board
   - **Windows**: Usually `COM3`, `COM4`, etc.
   - **Mac**: Usually `/dev/cu.usbserial-xxxx` or `/dev/cu.SLAB_USBtoUART`
   - **Linux**: Usually `/dev/ttyUSB0` or `/dev/ttyACM0`

### 7. Open and Verify the Sketch

1. Open `cyd_synth_test.ino` in Arduino IDE
2. Review the pin definitions at the top of the file:
   - Verify I²S pins match your wiring (IO21, IO22, IO35)
   - Verify touch pins match your CYD board
   - Adjust display library includes if needed

3. Click the **Verify** button (checkmark icon) to compile the sketch
4. Fix any compilation errors before proceeding

### 8. Upload the Sketch

1. Click the **Upload** button (arrow icon) in Arduino IDE
2. Wait for compilation to complete
3. The sketch will be uploaded to the ESP32
4. You should see "Hard resetting via RTS pin..." when done
5. The board will automatically reset and start running the sketch

### 9. Monitor Serial Output

1. Go to **Tools** > **Serial Monitor**
2. Set baud rate to **115200**
3. You should see:
   - "CYD Synthesizer Test Starting..."
   - "Initializing display..."
   - "Initializing touch screen..."
   - "Initializing I²S..."
   - "I²S initialized successfully"
   - "Audio task created on Core 0"
   - "Setup complete!"

### 10. Test the Functionality

1. **Audio Test**: You should hear a 440Hz sine wave (A4 note) from the 3.5mm jack
2. **Display Test**: The screen should show status information
3. **Touch Test**: Touch the screen - you should see:
   - Yellow circles at touch points
   - Touch coordinates in the status area
   - Serial output with touch coordinates

## Troubleshooting

### Board Not Recognized

- **Check USB cable**: Use a data-capable cable
- **Check drivers**: Install CP2102 or CH340 drivers if needed
- **Try different USB port**: Use a USB 2.0 port (not USB 3.0)
- **Check Device Manager**: Look for unknown devices or COM port issues

### Upload Fails

- **Hold BOOT button**: Some boards require holding the BOOT button during upload
- **Lower upload speed**: Try 921600, 460800, or 115200 in Tools > Upload Speed
- **Check connections**: Ensure USB cable is securely connected
- **Try manual reset**: Press RESET button on the board, then immediately click Upload

### Compilation Errors

- **Missing libraries**: Install all required libraries
- **Wrong board selected**: Ensure "ESP32 Dev Module" is selected
- **Library conflicts**: Check for multiple versions of the same library
- **Pin conflicts**: Verify pin definitions match your hardware

### No Audio Output

- **Check wiring**: Verify I²S pins (IO21, IO22, IO35) are correct
- **Check power**: Ensure DAC has 3.3V power and ground is connected
- **Check volume**: Ensure audio output device (speaker/headphones) is connected
- **Check Serial Monitor**: Look for I²S initialization errors
- **Verify I²S configuration**: Check sample rate and bit depth settings

### Display Not Working

- **Check TFT_eSPI configuration**: Verify User_Setup.h has correct pins
- **Check display pins**: Verify SPI pins match your CYD board
- **Check backlight**: Some displays need backlight pin configured
- **Try different rotation**: Change `tft.setRotation()` value (0-3)

### Touch Not Working

- **Check touch pins**: Verify TOUCH_CS and TOUCH_IRQ pins
- **Check touch library**: Ensure XPT2046_Touchscreen library is installed
- **Check rotation**: Touch rotation should match display rotation
- **Check calibration**: Touch coordinates may need calibration/adjustment

### Audio and Touch Interference

- **Check pin conflicts**: Ensure I²S pins don't conflict with touch pins
- **Adjust task priorities**: Audio task should have higher priority
- **Check power supply**: Ensure stable power to both ESP32 and DAC
- **See I2S_AUDIO_INTERFERENCE_NOTES.md** for detailed troubleshooting

## Additional Notes

- **First Upload**: The first upload may take longer as it partitions the flash
- **Subsequent Uploads**: Should be faster
- **Serial Monitor**: Keep Serial Monitor closed during upload, then open it after
- **Power**: The board can be powered via USB or external power (if supported)
- **Reset**: Press the RESET button on the board to restart the program

## Quick Reference

| Setting | Value |
|---------|-------|
| Board | ESP32 Dev Module |
| Upload Speed | 115200 |
| CPU Frequency | 240MHz |
| Flash Frequency | 80MHz |
| Flash Size | 4MB |
| Partition Scheme | Default 4MB with spiffs |
| Serial Monitor Baud | 115200 |

## Support

If you encounter issues:
1. Check the Serial Monitor for error messages
2. Review I2S_AUDIO_INTERFERENCE_NOTES.md for audio issues
3. Verify all pin connections match your hardware
4. Check library versions and compatibility
5. Try a simple blink sketch first to verify board connectivity

