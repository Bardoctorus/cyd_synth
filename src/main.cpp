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
#include "audio/lfo.h"
#include "audio/adsr.h"
#include "ui/pentatonic_ui.h"
#include "ui/parameter_control.h"

// Global hardware instances
Display display;
TouchScreen touch;
AudioOutput audio;
Oscillator oscillator;
BiquadFilter filterLeft;
BiquadFilter filterRight;
StereoDelay stereoDelay;  // Renamed from 'delay' to avoid conflict with Arduino delay() function
LFO delayLFO;  // LFO for modulating delay time
ADSR envelope;  // ADSR envelope for smooth note transitions
PentatonicUI pentatonicUI;
ParameterControl parameterControl;

// Audio generation state - volatile for thread safety (read by audio task)
volatile float currentFreq = 440.0;
volatile float currentFilterCutoff = FILTER_MIN_CUTOFF;
volatile bool isTouching = false;
volatile float currentDelayTimeMs = DELAY_TIME_MS_DEFAULT;  // Runtime-controllable delay time (smoothed)
volatile float targetDelayTimeMs = DELAY_TIME_MS_DEFAULT;   // Target delay time (from touch input)
volatile float actualDelayTimeMs = DELAY_TIME_MS_DEFAULT;   // Actual delay time being used (with LFO modulation)
volatile float currentDelayVarianceMs = DELAY_TIME_VARIANCE_MS;  // Runtime-controllable stereo variance
volatile float currentLFODepthMs = LFO_DEPTH_MS_DEFAULT;  // LFO depth (0-100ms)
volatile float targetLFODepthMs = LFO_DEPTH_MS_DEFAULT;    // Target LFO depth (from touch input)
volatile float actualLFODepthMs = LFO_DEPTH_MS_DEFAULT;   // Actual LFO depth being used (smoothed)
volatile float currentLFOSpeedHz = LFO_SPEED_HZ_DEFAULT;   // LFO speed (0.1-50 Hz)
volatile float targetLFOSpeedHz = LFO_SPEED_HZ_DEFAULT;    // Target LFO speed (from touch input)
volatile float actualLFOSpeedHz = LFO_SPEED_HZ_DEFAULT;    // Actual LFO speed being used (smoothed)
volatile int currentBaseNoteDetent = BASE_NOTE_DEFAULT;     // Base note detent (0-4)
volatile int targetBaseNoteDetent = BASE_NOTE_DEFAULT;     // Target base note detent (from touch input)
volatile int currentUpperToneDetent = UPPER_TONE_DEFAULT;  // Upper tone detent (0-4)
volatile int targetUpperToneDetent = UPPER_TONE_DEFAULT;  // Target upper tone detent (from touch input)

// FreeRTOS task handle
TaskHandle_t audioTaskHandle = NULL;

// Audio generation task (runs on Core 0)
void audioTask(void *parameter) {
  // Buffer for stereo I2S: 64 samples * 2 channels = 128 samples
  int16_t audioBuffer[128];  // Interleaved L/R samples for stereo
  
  Serial2.println("Audio task started - generating sine + sawtooth with delay and filter");
  Serial2.printf("Delay: %.0fms (default), Feedback: %.0f%%, Stereo width: %.1fms, Send: %.0f%%\n", 
                 DELAY_TIME_MS_DEFAULT, DELAY_FEEDBACK * 100.0, DELAY_TIME_VARIANCE_MS, DELAY_SEND_LEVEL * 100.0);
  Serial2.printf("Delay sample rate: %d Hz (Audio: %d Hz)\n", DELAY_SAMPLE_RATE, SAMPLE_RATE);
  Serial2.printf("Delay buffer size: %d samples (%.2f KB)\n", DELAY_BUFFER_SIZE, (DELAY_BUFFER_SIZE * 2) / 1024.0);
  
  // Initialize delay
  stereoDelay.init();
  
  // Initialize oscillator with default semitone interval (12 semitones = octave)
  oscillator.setSemitoneInterval(UPPER_TONE_SEMITONES[UPPER_TONE_DEFAULT]);
  
  unsigned long lastDebugTime = 0;
  uint32_t totalBytesWritten = 0;
  
  while (true) {
    // Read current state (read volatile once per buffer)
    float freq = currentFreq;
    float cutoff = currentFilterCutoff;
    bool touching = isTouching;
    float delayVariance = currentDelayVarianceMs;
    
    // Smooth delay time changes (gradual transition to prevent clicks)
    // Smoothing rate: 0.01 means 1% per sample toward target (very smooth, ~100ms transition)
    float delayTimeTarget = targetDelayTimeMs;
    float delayTimeSmoothing = 0.01;  // Adjust this: lower = smoother but slower, higher = faster but less smooth
    currentDelayTimeMs += (delayTimeTarget - currentDelayTimeMs) * delayTimeSmoothing;
    float baseDelayTime = currentDelayTimeMs;
    
    // Smooth LFO parameters
    float lfoDepthSmoothing = 0.01;
    currentLFODepthMs += (targetLFODepthMs - currentLFODepthMs) * lfoDepthSmoothing;
    float lfoDepth = currentLFODepthMs;
    
    float lfoSpeedSmoothing = 0.01;
    currentLFOSpeedHz += (targetLFOSpeedHz - currentLFOSpeedHz) * lfoSpeedSmoothing;
    float lfoSpeed = currentLFOSpeedHz;
    
    // Update LFO speed
    delayLFO.setSpeed(lfoSpeed);
    
    // Apply LFO modulation to delay time
    // LFO output is -1.0 to 1.0 (sine wave)
    // Depth represents total range: depth=100ms means ±50ms modulation
    // So we scale by half the depth to get symmetric modulation around base
    float lfoValue = delayLFO.process();
    float modulationAmount = lfoValue * (lfoDepth / 2.0);  // Half depth for ±range
    float delayTime = baseDelayTime + modulationAmount;
    
    // Clamp delay time to valid range (will hold at min/max until LFO brings it back in range)
    delayTime = constrain(delayTime, DELAY_TIME_MS_MIN, DELAY_TIME_MS_MAX);
    
    // Store actual values being used (for display)
    actualDelayTimeMs = delayTime;
    actualLFODepthMs = lfoDepth;
    actualLFOSpeedHz = lfoSpeed;
    
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
    
    // Update sawtooth interval from upper tone detent (only when it changes)
    static int lastUpperToneDetent = -1;
    if (currentUpperToneDetent != lastUpperToneDetent) {
      float semitoneInterval = UPPER_TONE_SEMITONES[currentUpperToneDetent];
      oscillator.setSemitoneInterval(semitoneInterval);
      lastUpperToneDetent = currentUpperToneDetent;
    }
    
    for (int i = 0; i < 64; i++) {
      // STEP 1: Generate instrument signal (always generate, envelope will control amplitude)
      // Note: freq is already quantized to pentatonic scale with current base frequency
      // Always generate (even when not touching) so phases stay continuous - envelope handles amplitude
      float rawInstrumentSignal = oscillator.generate(freq, true);  // Always active, envelope controls amplitude
      
      // STEP 1.5: Apply ADSR envelope for smooth attack/release (prevents pops/cracks)
      float envelopeValue = envelope.process(touching);
      float instrumentSignal = rawInstrumentSignal * envelopeValue;
      
      // STEP 2: Apply filter to instrument signal -> filtered instrument (instrument channel)
      float filteredInstrumentLeft = filterLeft.process(instrumentSignal);
      float filteredInstrumentRight = filterRight.process(instrumentSignal);
      
      // STEP 3 & 4: Process delay (send filtered instrument, get delay return)
      // Pass runtime-controllable delay time and variance
      float delayReturnLeft, delayReturnRight;
      stereoDelay.process(filteredInstrumentLeft, filteredInstrumentRight, delayReturnLeft, delayReturnRight, delayTime, delayVariance);
      
      // STEP 5: Mix filtered instrument (dry, only when touching) + delay return (always) -> master output
      float masterLeft = filteredInstrumentLeft + delayReturnLeft;
      float masterRight = filteredInstrumentRight + delayReturnRight;
      
      // Apply master gain reduction to prevent excessive peaks (gain staging)
      // This reduces the signal level before the final limiter to avoid harsh clipping
      masterLeft *= MASTER_GAIN;
      masterRight *= MASTER_GAIN;
      
      // STEP 6: Apply hard limiter (safety protection, should rarely trigger with proper gain staging)
      masterLeft = constrain(masterLeft, -1.0, 1.0);
      masterRight = constrain(masterRight, -1.0, 1.0);
      
      // STEP 7: Send to DAC (interleaved Left and Right)
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
  
  // Draw parameter controls in bottom half
  parameterControl.drawControls(display.getTFT());
  
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
  
  int screenHeight = display.getTFT().height();
  int screenWidth = display.getTFT().width();
  int controlAreaHeight = screenHeight / 2;
  bool inTopArea = touchData.active && (touchData.y < controlAreaHeight);
  bool inBottomArea = touchData.active && (touchData.y >= controlAreaHeight);
  
  if (inTopArea) {
    // Top half: Pitch and filter control
    // X-axis controls pitch: quantized to pentatonic scale
    float freq = pentatonicUI.touchToFrequency(touchData.x, screenWidth);
    currentFreq = freq;
    
    // Y-axis controls low-pass filter cutoff (only top half of screen)
    float filterCutoff = pentatonicUI.touchToFilterCutoff(touchData.y, screenHeight);
    currentFilterCutoff = filterCutoff;
    
    // Determine which pentatonic zone we're in
    int zoneIndex = pentatonicUI.getZone(freq);
    
    // Update display with zone highlighting
    pentatonicUI.updateDisplay(display.getTFT(), true, zoneIndex, freq);
    
    isTouching = true;  // Set touch flag for audio task
  } else if (inBottomArea) {
    // Bottom half: Parameter controls (delay time, LFO depth, LFO speed)
    // X-axis determines which parameter (0=delay, 1=LFO depth, 2=LFO speed)
    // Y-axis controls the value (vertical sliders)
    ParameterType param = parameterControl.touchToParameter(touchData.x, touchData.y, screenWidth, screenHeight);
    
    if (param == PARAM_DELAY_TIME) {
      float delayTime = parameterControl.touchToDelayTime(touchData.y, screenHeight);
      targetDelayTimeMs = delayTime;  // Set target, smoothing will happen in audio task
    } else if (param == PARAM_LFO_DEPTH) {
      float lfoDepth = parameterControl.touchToLFODepth(touchData.y, screenHeight);
      targetLFODepthMs = lfoDepth;  // Set target, smoothing will happen in audio task
    } else if (param == PARAM_LFO_SPEED) {
      float lfoSpeed = parameterControl.touchToLFOSpeed(touchData.y, screenHeight);
      targetLFOSpeedHz = lfoSpeed;  // Set target, smoothing will happen in audio task
    } else if (param == PARAM_BASE_NOTE) {
      int baseNoteDetent = parameterControl.touchToBaseNote(touchData.y, screenHeight);
      targetBaseNoteDetent = baseNoteDetent;  // Update immediately (no smoothing needed for detents)
      currentBaseNoteDetent = baseNoteDetent;  // Update immediately
      
      // Update base frequency for pentatonic scale
      int stepsFromMiddle = baseNoteDetent - BASE_NOTE_DEFAULT;
      float newBaseFreq = MIN_FREQ_BASE * pow(WHOLE_TONE_RATIO, stepsFromMiddle);
      pentatonicUI.setBaseFrequency(newBaseFreq);
    } else if (param == PARAM_UPPER_TONE) {
      int upperToneDetent = parameterControl.touchToUpperTone(touchData.y, screenHeight);
      targetUpperToneDetent = upperToneDetent;  // Update immediately (no smoothing needed for detents)
      currentUpperToneDetent = upperToneDetent;  // Update immediately
    }
    
    // Not generating audio when adjusting parameters
    isTouching = false;
  } else {
    // No touch - keep filter at last position (sound continues via delay)
    isTouching = false;  // Clear touch flag (stop generating new audio, but delay continues)
    pentatonicUI.updateDisplay(display.getTFT(), false, -1, 0.0);
  }
  
  // Update display periodically (separate from touch polling for performance)
  display.update();
  
  // Update parameter display periodically to show current values (use smoothed values for smooth slider)
  static unsigned long lastParamUpdate = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastParamUpdate >= DISPLAY_UPDATE_INTERVAL) {
    lastParamUpdate = currentTime;
    // Use smoothed values for UI display (smooth slider movement)
    // Show actual values from audio task (with LFO modulation applied)
    parameterControl.updateParameterDisplay(display.getTFT(), PARAM_DELAY_TIME, 
                                             parameterControl.getSmoothedDelayTime(), actualDelayTimeMs);
    parameterControl.updateParameterDisplay(display.getTFT(), PARAM_LFO_DEPTH, 
                                             parameterControl.getSmoothedLFODepth(), actualLFODepthMs);
    parameterControl.updateParameterDisplay(display.getTFT(), PARAM_LFO_SPEED, 
                                             parameterControl.getSmoothedLFOSpeed(), actualLFOSpeedHz);
    parameterControl.updateParameterDisplay(display.getTFT(), PARAM_BASE_NOTE, 
                                             (float)parameterControl.getBaseNoteDetent(), (float)currentBaseNoteDetent);
    parameterControl.updateParameterDisplay(display.getTFT(), PARAM_UPPER_TONE, 
                                             (float)parameterControl.getUpperToneDetent(), (float)currentUpperToneDetent);
  }
  
  // Minimal delay - touch polling is priority for smooth audio
  // Use yield() instead of delay() to allow other tasks to run
  yield();
}
