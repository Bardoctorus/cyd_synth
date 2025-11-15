/*
 * CYD Synthesizer Test - 440Hz Sine Wave + Display + Touch
 * 
 * Hardware:
 * - ESP32-2432S028R (Cheap Yellow Display)
 * - PCM5102 DAC
 * - I²S Connections: IO1=BCK (P1), IO22=DIN (P3/CN1), IO27=LCK (CN1)
 *   NOTE: IO35 is INPUT ONLY (cannot be used for I2S output)
 *   NOTE: IO21 is used for TFT backlight (conflicts with I2S)
 * 
 * This sketch:
 * - Generates a 440Hz sine wave and outputs via I²S to PCM5102 DAC
 * - Displays status information on the TFT screen
 * - Tests touchscreen functionality with visual feedback
 */

#include <Arduino.h>
#include <driver/i2s.h>
#include <math.h>
#include <SPI.h>

// Display library - TFT_eSPI (configured via platformio.ini build flags)
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

// Touch library - XPT2046
#include <XPT2046_Touchscreen.h>

// Touchscreen pins for CYD board (ESP32-2432S028R)
// These are different from the display SPI pins - touchscreen uses its own SPI bus
#define XPT2046_IRQ 36   // T_IRQ pin
#define XPT2046_MOSI 32  // T_DIN pin
#define XPT2046_MISO 39  // T_OUT pin
#define XPT2046_CLK 25   // T_CLK pin
#define XPT2046_CS 33    // T_CS pin

// Create separate SPI instance for touchscreen (uses VSPI)
SPIClass touchscreenSPI = SPIClass(VSPI);

// Initialize touchscreen with CS and IRQ pins
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

// I²S Configuration for PCM5102 DAC
// Using only pins available on JST connectors P3, CN1, and P1 (non-invasive)
// NOTE: IO35 is INPUT ONLY - cannot be used for I2S output!
// NOTE: IO21 is used for TFT backlight - conflicts with I2S!
// Pins available on connectors:
//   P3: GND, IO35 (INPUT), IO22, IO21 (TFT backlight)
//   CN1: GND, IO22, IO27, 3.3V
//   P1: VIN, IO1 (TX), IO3 (RX), GND
#define I2S_NUM         I2S_NUM_0
#define I2S_BCK_PIN     1   // Bit Clock - IO1 (TX) from P1 connector (YellowBlack wire)
                          // Serial switched to Serial2 to free IO1 for I2S
#define I2S_LRCK_PIN    27  // Word Select / Left-Right Clock - IO27 from CN1 connector (Yellow wire)
#define I2S_DATA_PIN    22  // Data Input - IO22 from CN1 connector (Blue wire)

// Audio parameters
#define SAMPLE_RATE     44100
#define SINE_FREQ       440.0  // A4 note
#define AMPLITUDE       16000  // Max amplitude for 16-bit (32767 max, using 16000 for headroom)

// Audio generation state
float phase = 0.0;
float phase_increment = 2.0 * PI * SINE_FREQ / SAMPLE_RATE;

// Display update timing
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 100; // Update display every 100ms

// Touch state
bool touchDetected = false;
int touchX = 0;
int touchY = 0;

// FreeRTOS task handles
TaskHandle_t audioTaskHandle = NULL;

// Audio generation task (runs on Core 0)
void audioTask(void *parameter) {
  // Buffer for stereo I2S: 64 samples * 2 channels = 128 samples
  int16_t audioBuffer[128];  // Interleaved L/R samples for stereo
  
  Serial2.println("Audio task started - generating 440Hz sine wave");
  unsigned long lastDebugTime = 0;
  uint32_t totalBytesWritten = 0;
  
  while (true) {
    // Generate sine wave samples for both left and right channels
    for (int i = 0; i < 64; i++) {
      int16_t sample = (int16_t)(AMPLITUDE * sin(phase));
      phase += phase_increment;
      if (phase >= 2.0 * PI) {
        phase -= 2.0 * PI;
      }
      
      // Interleave: Left sample, Right sample
      audioBuffer[i * 2] = sample;     // Left channel
      audioBuffer[i * 2 + 1] = sample;  // Right channel (same for mono)
    }
    
    // Send to I²S (128 samples * 2 bytes = 256 bytes)
    size_t bytesWritten;
    esp_err_t err = i2s_write(I2S_NUM, audioBuffer, sizeof(audioBuffer), &bytesWritten, portMAX_DELAY);
    
    if (err != ESP_OK) {
      Serial2.printf("I2S write error: %d\n", err);
    }
    
    totalBytesWritten += bytesWritten;
    
    // Debug output every 5 seconds
    unsigned long now = millis();
    if (now - lastDebugTime > 5000) {
      Serial2.printf("Audio: %lu bytes written, %lu bytes/sec\n", 
                    totalBytesWritten, totalBytesWritten / ((now / 1000) + 1));
      lastDebugTime = now;
    }
    
    // No delay - PCM5102 needs continuous data stream
    // The I2S driver handles timing internally
  }
}

void setup() {
  // Use Serial2 for debugging to free up IO1 (TX) for I2S BCK
  // Serial2 uses IO17 (TX) and IO16 (RX) by default on ESP32
  Serial2.begin(115200);
  delay(1000);
  
  Serial2.println("CYD Synthesizer Test Starting...");
  Serial2.println("Note: Using Serial2 for debug output (IO17/IO16)");
  Serial2.println("IO1 (P1) is now used for I2S BCK");
  
  // Initialize display
  Serial2.println("Initializing display...");
  tft.init();
  tft.setRotation(1);  // Adjust rotation as needed (0-3)
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("CYD Synthesizer");
  tft.setTextSize(1);
  tft.setCursor(10, 40);
  tft.println("440Hz Sine Wave Test");
  tft.setCursor(10, 60);
  tft.println("Touch screen to test");
  
  // Initialize touch screen
  Serial2.println("Initializing touch screen...");
  Serial2.printf("Touch pins: CS=%d, IRQ=%d, MOSI=%d, MISO=%d, CLK=%d\n", 
                XPT2046_CS, XPT2046_IRQ, XPT2046_MOSI, XPT2046_MISO, XPT2046_CLK);
  
  // Start separate SPI bus for touchscreen with explicit pins
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  
  // Initialize touchscreen with the SPI instance
  ts.begin(touchscreenSPI);
  ts.setRotation(1);  // Match display rotation (landscape mode)
  
  Serial2.println("Touch screen initialized successfully");
  
  // Initialize I²S for PCM5102 DAC
  Serial2.println("Initializing I²S for PCM5102...");
  
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,  // Standard I2S format for PCM5102
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,  // High priority interrupt
    .dma_buf_count = 8,  // More buffers for smoother streaming
    .dma_buf_len = 128,  // 64 samples * 2 channels = 128 samples
    .use_apll = true,  // Try without APLL first - some PCM5102 modules are sensitive to clock
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };
  
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCK_PIN,
    .ws_io_num = I2S_LRCK_PIN,
    .data_out_num = I2S_DATA_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  
  // Install and start I²S driver
  esp_err_t err = i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial2.printf("I²S driver install failed: %d\n", err);
    tft.setCursor(10, 80);
    tft.setTextColor(TFT_RED);
    tft.println("I2S ERROR!");
    while (1) delay(1000);
  }
  
  err = i2s_set_pin(I2S_NUM, &pin_config);
  if (err != ESP_OK) {
    Serial2.printf("I²S pin config failed: %d\n", err);
    tft.setCursor(10, 80);
    tft.setTextColor(TFT_RED);
    tft.println("I2S PIN ERROR!");
    while (1) delay(1000);
  }
  
  // Clear DMA buffer and start I²S
  i2s_zero_dma_buffer(I2S_NUM);
  
  // Explicitly start I2S (some configurations need this)
  err = i2s_start(I2S_NUM);
  if (err != ESP_OK) {
    Serial2.printf("I²S start failed: %d\n", err);
    tft.setCursor(10, 80);
    tft.setTextColor(TFT_RED);
    tft.println("I2S START ERROR!");
    tft.setTextColor(TFT_WHITE);
  }
  
  Serial2.println("I²S initialized and started successfully");
  Serial2.printf("I2S Config: Sample Rate=%d Hz, Bits=%d, Format=I2S_STAND, APLL=%s\n", 
                SAMPLE_RATE, 16, i2s_config.use_apll ? "ON" : "OFF");
  Serial2.printf("I2S Pins: BCK=%d (P1), LRCK=%d (CN1), DATA=%d (P3/CN1)\n", I2S_BCK_PIN, I2S_LRCK_PIN, I2S_DATA_PIN);
  Serial2.printf("DMA: %d buffers of %d samples each\n", i2s_config.dma_buf_count, i2s_config.dma_buf_len);
  tft.setCursor(10, 100);
  tft.setTextColor(TFT_GREEN);
  tft.println("I2S: OK");
  tft.setTextColor(TFT_WHITE);
  
  // Create audio task on Core 0 (high priority)
  xTaskCreatePinnedToCore(
    audioTask,        // Task function
    "AudioTask",      // Task name
    4096,             // Stack size
    NULL,             // Parameters
    1,                // Priority (1 is higher than default)
    &audioTaskHandle, // Task handle
    0                 // Core 0 (audio processing)
  );
  
  Serial2.println("Audio task created on Core 0");
  tft.setCursor(10, 120);
  tft.setTextColor(TFT_GREEN);
  tft.println("Audio: RUNNING");
  tft.setTextColor(TFT_WHITE);
  
  delay(500);
  
  Serial2.println("Setup complete!");
}

void loop() {
  // This loop runs on Core 1 (UI and touch handling)
  
  // Check for touch input - use both tirqTouched() and touched() for reliable detection
  // tirqTouched() checks the IRQ pin, touched() reads the touch controller
  if (ts.tirqTouched() && ts.touched()) {
    TS_Point p = ts.getPoint();
    
    // Debug: Print raw touch values
    Serial2.printf("Raw touch: X=%d, Y=%d, Z=%d\n", p.x, p.y, p.z);
    
    // Convert touch coordinates to display coordinates
    // Calibration values from Random Nerd Tutorials example for CYD board
    // These values work better than 0-4095 mapping for the CYD touchscreen
    touchX = map(p.x, 200, 3700, 1, tft.width());
    touchY = map(p.y, 240, 3800, 1, tft.height());
    
    touchDetected = true;
    
    // Visual feedback - draw a circle at touch point
    tft.fillCircle(touchX, touchY, 10, TFT_YELLOW);
    delay(50);
    tft.fillCircle(touchX, touchY, 10, TFT_BLACK);
    
    Serial2.printf("Touch detected: X=%d, Y=%d (mapped), Pressure=%d\n", touchX, touchY, p.z);
    
    delay(100);  // Debounce delay
  } else {
    touchDetected = false;
  }
  
  // Update display periodically
  unsigned long currentTime = millis();
  if (currentTime - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    lastDisplayUpdate = currentTime;
    
    // Update status area
    tft.fillRect(10, 140, 200, 60, TFT_BLACK);
    tft.setCursor(10, 140);
    tft.setTextSize(1);
    tft.setTextColor(TFT_CYAN);
    tft.printf("Freq: %.1f Hz", SINE_FREQ);
    tft.setCursor(10, 155);
    tft.printf("Sample Rate: %d Hz", SAMPLE_RATE);
    tft.setCursor(10, 170);
    tft.printf("Uptime: %lu s", currentTime / 1000);
    
    if (touchDetected) {
      tft.setCursor(10, 185);
      tft.setTextColor(TFT_YELLOW);
      tft.printf("Touch: X=%d Y=%d", touchX, touchY);
    } else {
      tft.setCursor(10, 185);
      tft.setTextColor(TFT_DARKGREY);
      tft.println("Touch: None");
    }
  }
  
  // Small delay to prevent watchdog issues
  delay(10);
}

