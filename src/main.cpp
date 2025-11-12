/*
 * CYD Synthesizer Test - 440Hz Sine Wave + Display + Touch
 * 
 * Hardware:
 * - ESP32-2432S028R (Cheap Yellow Display)
 * - PCM1502 DAC
 * - I²S Connections: IO21=BCK, IO22=DIN, IO35=LCK
 * 
 * This sketch:
 * - Generates a 440Hz sine wave and outputs via I²S to PCM1502 DAC
 * - Displays status information on the TFT screen
 * - Tests touchscreen functionality with visual feedback
 */

#include <Arduino.h>
#include <driver/i2s.h>
#include <math.h>

// Display library - TFT_eSPI (configured via platformio.ini build flags)
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

// Touch library - XPT2046
#include <XPT2046_Touchscreen.h>

// Touch pins - adjust these based on your CYD board
#define TOUCH_CS  5
#define TOUCH_IRQ 25  // Optional, can use -1 if not connected

XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

// I²S Configuration for PCM1502
#define I2S_NUM         I2S_NUM_0
#define I2S_BCK_PIN     21
#define I2S_LRCK_PIN    35  // Word Select / Left-Right Clock
#define I2S_DATA_PIN    22  // Data Input to DAC

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
  int16_t audioBuffer[64];  // Small buffer for low latency
  
  while (true) {
    // Generate sine wave samples
    for (int i = 0; i < 64; i++) {
      audioBuffer[i] = (int16_t)(AMPLITUDE * sin(phase));
      phase += phase_increment;
      if (phase >= 2.0 * PI) {
        phase -= 2.0 * PI;
      }
    }
    
    // Send to I²S
    size_t bytesWritten;
    i2s_write(I2S_NUM, audioBuffer, sizeof(audioBuffer), &bytesWritten, portMAX_DELAY);
    
    // Small delay to prevent watchdog issues
    vTaskDelay(1);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("CYD Synthesizer Test Starting...");
  
  // Initialize display
  Serial.println("Initializing display...");
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
  Serial.println("Initializing touch screen...");
  ts.begin();
  ts.setRotation(1);  // Match display rotation
  
  // Initialize I²S for PCM1502 DAC
  Serial.println("Initializing I²S...");
  
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,  // High priority interrupt
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,  // Use APLL for better clock accuracy if needed
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
    Serial.printf("I²S driver install failed: %d\n", err);
    tft.setCursor(10, 80);
    tft.setTextColor(TFT_RED);
    tft.println("I2S ERROR!");
    while (1) delay(1000);
  }
  
  err = i2s_set_pin(I2S_NUM, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("I²S pin config failed: %d\n", err);
    tft.setCursor(10, 80);
    tft.setTextColor(TFT_RED);
    tft.println("I2S PIN ERROR!");
    while (1) delay(1000);
  }
  
  // Start I²S
  i2s_zero_dma_buffer(I2S_NUM);
  
  Serial.println("I²S initialized successfully");
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
  
  Serial.println("Audio task created on Core 0");
  tft.setCursor(10, 120);
  tft.setTextColor(TFT_GREEN);
  tft.println("Audio: RUNNING");
  tft.setTextColor(TFT_WHITE);
  
  delay(500);
  
  Serial.println("Setup complete!");
}

void loop() {
  // This loop runs on Core 1 (UI and touch handling)
  
  // Check for touch input
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    
    // Convert touch coordinates to display coordinates
    // Note: Touch coordinates may need calibration/adjustment
    touchX = map(p.x, 0, 4095, 0, tft.width());
    touchY = map(p.y, 0, 4095, 0, tft.height());
    
    // Invert Y if needed (depends on rotation)
    touchY = tft.height() - touchY;
    
    touchDetected = true;
    
    // Visual feedback - draw a circle at touch point
    tft.fillCircle(touchX, touchY, 10, TFT_YELLOW);
    delay(50);
    tft.fillCircle(touchX, touchY, 10, TFT_BLACK);
    
    Serial.printf("Touch detected: X=%d, Y=%d\n", touchX, touchY);
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
      tft.setTextColor(TFT_GRAY);
      tft.println("Touch: None");
    }
  }
  
  // Small delay to prevent watchdog issues
  delay(10);
}

