/*
 * CYD Synthesizer - Main Entry Point
 * 
 * Hardware:
 * - ESP32-2432S028R (Cheap Yellow Display)
 * - PCM5102 DAC
 * - I²S Connections: IO1=BCK (P1), IO22=DIN (P3/CN1), IO27=LCK (CN1)
 * 
 * This sketch:
 * - Generates dual oscillator (sine + sawtooth) and outputs via I²S to PCM5102 DAC
 * - Touchscreen control:
 *   - X-axis (left-right): Controls pitch (pentatonic scale, 80Hz-2kHz)
 *   - Y-axis (up-down): Controls low-pass filter cutoff (40Hz-4kHz)
 * - Displays pentatonic zones and touch feedback
 */

#include <Arduino.h>
#include <HardwareSerial.h>
#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>
#include "config.h"
#include "hardware/display.h"
#include "hardware/touch.h"
#include "hardware/audio_output.h"
#include "audio/oscillator.h"
#include "audio/filter.h"
#include "audio/delay.h"
#include "ui/pentatonic_ui.h"

// Global hardware instances
Display display;
TouchScreen touch;
AudioOutput audio;
Oscillator oscillator;
BiquadFilter filterLeft;
BiquadFilter filterRight;
StereoDelay stereoDelay;  // Renamed from 'delay' to avoid conflict with Arduino delay() function
PentatonicUI pentatonicUI;

// Audio generation state - volatile for thread safety (read by audio task)
volatile float currentFreq = 440.0;
volatile float currentFilterCutoff = FILTER_MIN_CUTOFF;
volatile bool isTouching = false;

// FreeRTOS task handle
TaskHandle_t audioTaskHandle = NULL;

// Audio generation task (runs on Core 0)
void audioTask(void *parameter) {
  // Buffer for stereo I2S: 64 samples * 2 channels = 128 samples
  int16_t audioBuffer[128];  // Interleaved L/R samples for stereo
  
  Serial2.println("Audio task started - generating sine + sawtooth with delay and filter");
  Serial2.printf("Delay: %.0fms, Feedback: %.0f%%, Stereo width: %.1fms, Send: %.0f%%\n", 
                 DELAY_TIME_MS, DELAY_FEEDBACK * 100.0, DELAY_TIME_VARIANCE_MS, DELAY_SEND_LEVEL * 100.0);
  Serial2.printf("Delay sample rate: %d Hz (Audio: %d Hz)\n", DELAY_SAMPLE_RATE, SAMPLE_RATE);
  Serial2.printf("Delay buffer size: %d samples (%.2f KB)\n", DELAY_BUFFER_SIZE, (DELAY_BUFFER_SIZE * 2) / 1024.0);
  
  // Initialize delay
  stereoDelay.init();
  
  unsigned long lastDebugTime = 0;
  uint32_t totalBytesWritten = 0;
  
  while (true) {
    // Read current state (read volatile once per buffer)
    float freq = currentFreq;
    float cutoff = currentFilterCutoff;
    bool touching = isTouching;
    
    // Update filter cutoff
    filterLeft.setCutoff(cutoff);
    filterRight.setCutoff(cutoff);
    
    // Audio processing flow (mixing desk architecture):
    // 1. Generate sine + sawtooth waves (ONLY when touching) -> instrument signal
    // 2. Apply filter to instrument signal -> filtered instrument (instrument channel)
    // 3. Send filtered instrument to delay (send level) -> delay input
    // 4. Delay processes and outputs -> delay return (no filter on delay)
    // 5. Mix: filtered instrument (dry, only when touching) + delay return (always) -> master output
    // 6. Send to DAC (Left and Right channels)
    
    for (int i = 0; i < 64; i++) {
      // STEP 1: Generate instrument signal (only when touching)
      float instrumentSignal = oscillator.generate(freq, touching);
      
      // STEP 2: Apply filter to instrument signal -> filtered instrument (instrument channel)
      float filteredInstrumentLeft = filterLeft.process(instrumentSignal);
      float filteredInstrumentRight = filterRight.process(instrumentSignal);
      
      // STEP 3 & 4: Process delay (send filtered instrument, get delay return)
      float delayReturnLeft, delayReturnRight;
      stereoDelay.process(filteredInstrumentLeft, filteredInstrumentRight, delayReturnLeft, delayReturnRight);
      
      // STEP 5: Mix filtered instrument (dry, only when touching) + delay return (always) -> master output
      float masterLeft = filteredInstrumentLeft + delayReturnLeft;
      float masterRight = filteredInstrumentRight + delayReturnRight;
      
      // STEP 6: Send to DAC (interleaved Left and Right)
      audioBuffer[i * 2] = (int16_t)(masterLeft * FIXED_AMPLITUDE);
      audioBuffer[i * 2 + 1] = (int16_t)(masterRight * FIXED_AMPLITUDE);
    }
    
    // Send buffer to I²S (128 samples * 2 bytes = 256 bytes)
    audio.writeBuffer(audioBuffer, 128);
    totalBytesWritten += 256;
    
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
  
  Serial2.println("CYD Synthesizer Starting...");
  Serial2.println("Note: Using Serial2 for debug output (IO17/IO16)");
  Serial2.println("IO1 (P1) is used for I2S BCK");
  
  // Initialize display
  Serial2.println("Initializing display...");
  display.init();
  
  // Initialize touch screen
  Serial2.println("Initializing touch screen...");
  Serial2.printf("Touch pins: CS=%d, IRQ=%d, MOSI=%d, MISO=%d, CLK=%d\n", 
                XPT2046_CS, XPT2046_IRQ, XPT2046_MOSI, XPT2046_MISO, XPT2046_CLK);
  touch.init();
  Serial2.println("Touch screen initialized successfully");
  
  // Initialize I²S for PCM5102 DAC
  Serial2.println("Initializing I²S for PCM5102...");
  if (!audio.init()) {
    Serial2.println("I²S initialization failed!");
    display.getTFT().setCursor(10, 80);
    display.getTFT().setTextColor(TFT_RED);
    display.getTFT().println("I2S ERROR!");
    while (1) delay(1000);
  }
  
  Serial2.println("I²S initialized and started successfully");
  Serial2.printf("I2S Config: Sample Rate=%d Hz, Bits=%d, Format=I2S_STAND, APLL=ON\n", SAMPLE_RATE, 16);
  Serial2.printf("I2S Pins: BCK=%d (P1), LRCK=%d (CN1), DATA=%d (P3/CN1)\n", I2S_BCK_PIN, I2S_LRCK_PIN, I2S_DATA_PIN);
  
  // Initialize filter coefficients (starts closed at min cutoff)
  filterLeft.setCutoff(FILTER_MIN_CUTOFF);
  filterRight.setCutoff(FILTER_MIN_CUTOFF);
  Serial2.printf("Low-pass filter initialized: Cutoff=%.0f Hz, Q=%.3f, GainComp=%.2f\n", 
                FILTER_MIN_CUTOFF, FILTER_Q, FILTER_GAIN_COMP);
  
  display.getTFT().setCursor(10, 100);
  display.getTFT().setTextColor(TFT_GREEN);
  display.getTFT().println("I2S: OK");
  
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
  display.getTFT().setCursor(10, 120);
  display.getTFT().setTextColor(TFT_GREEN);
  display.getTFT().println("Audio: RUNNING");
  
  delay(500);
  
  Serial2.println("Setup complete!");
}

void loop() {
  // This loop runs on Core 1 (UI and touch handling)
  // Touch polling is done as fast as possible for smooth audio response
  // Display updates are separate and less frequent
  
  // Read touch input
  TouchData touchData = touch.read(display.getTFT().width(), display.getTFT().height());
  
  if (touchData.active) {
    // X-axis controls pitch: quantized to pentatonic scale
    float freq = pentatonicUI.touchToFrequency(touchData.x, display.getTFT().width());
    currentFreq = freq;
    
    // Y-axis controls low-pass filter cutoff
    float filterCutoff = pentatonicUI.touchToFilterCutoff(touchData.y, display.getTFT().height());
    currentFilterCutoff = filterCutoff;
    
    // Determine which pentatonic zone we're in
    int zoneIndex = pentatonicUI.getZone(freq);
    
    // Update display with zone highlighting
    pentatonicUI.updateDisplay(display.getTFT(), true, zoneIndex, freq);
    
    isTouching = true;  // Set touch flag for audio task
  } else {
    // No touch - keep filter at last position (sound continues via delay)
    isTouching = false;  // Clear touch flag (stop generating new audio, but delay continues)
    pentatonicUI.updateDisplay(display.getTFT(), false, -1, 0.0);
  }
  
  // Update display periodically (separate from touch polling for performance)
  display.update();
  
  // Minimal delay - touch polling is priority for smooth audio
  // Use yield() instead of delay() to allow other tasks to run
  yield();
}
