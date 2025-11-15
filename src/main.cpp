/*
 * CYD Synthesizer - Touch-Controlled Sine Wave Generator
 * 
 * Hardware:
 * - ESP32-2432S028R (Cheap Yellow Display)
 * - PCM5102 DAC
 * - I²S Connections: IO1=BCK (P1), IO22=DIN (P3/CN1), IO27=LCK (CN1)
 *   NOTE: IO35 is INPUT ONLY (cannot be used for I2S output)
 *   NOTE: IO21 is used for TFT backlight (conflicts with I2S)
 * 
 * This sketch:
 * - Generates sine wave and outputs via I²S to PCM5102 DAC
 * - Touchscreen control:
 *   - X-axis (left-right): Controls pitch from 60Hz to 12kHz
 *   - Y-axis (up-down): Controls volume from 0% to 99%
 * - Displays current frequency, volume, and touch position
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
#define MIN_FREQ        80.0   // Minimum frequency (Hz) for pitch control - pentatonic scale
#define MAX_FREQ        2000.0 // Maximum frequency (Hz) for pitch control - pentatonic scale
#define MAX_AMPLITUDE   32767   // Max amplitude for 16-bit (32767 max, using 16000 for headroom)
#define FIXED_VOLUME_PERCENT 40.0  // Fixed volume percentage
#define FIXED_AMPLITUDE ((FIXED_VOLUME_PERCENT / 100.0) * MAX_AMPLITUDE)

// Delay parameters
#define DELAY_TIME_MS    250.0  // Delay time in milliseconds (reduced from 400ms for memory)
#define DELAY_FEEDBACK   0.8    // Feedback level (50%)
#define DELAY_TIME_VARIANCE_MS 20.0  // Stereo width: time difference between L and R channels (reduced from 55ms for memory)
#define DELAY_SEND_LEVEL 0.5    // Send level: how much of the filtered instrument goes to delay (0.0 to 1.0)

// Delay sample rate: can be set to 22050 for lower memory usage (A/B test)
// Set to SAMPLE_RATE for full quality, or 22050 for half memory usage
#define DELAY_SAMPLE_RATE 22050  // Change to SAMPLE_RATE to use full quality delay
// #define DELAY_SAMPLE_RATE SAMPLE_RATE  // Uncomment this and comment above for full quality

#define DELAY_BUFFER_SIZE ((int)((DELAY_TIME_MS + DELAY_TIME_VARIANCE_MS) * DELAY_SAMPLE_RATE / 1000.0) + 1)  // Buffer size in samples

// Filter parameters
#define FILTER_MIN_CUTOFF  40.0   // Minimum filter cutoff (Hz) - below this kills sound completely
#define FILTER_MAX_CUTOFF  2000.0  // Maximum filter cutoff (Hz) - above this doesn't affect sound
#define FILTER_Q           2.5     // Filter Q (resonance) - reduced to avoid sudden volume jumps
#define FILTER_GAIN_COMP   0.7     // Gain compensation to prevent resonance from causing volume jumps

// Dead zone sizes (in pixels from edges)
#define DEAD_ZONE_SIZE  20  // Pixels from top/bottom for dead zones

// Audio generation state - volatile for thread safety (read by audio task)
volatile float currentFreq = 440.0;  // Current frequency (Hz) - pitch control
volatile float currentFilterCutoff = FILTER_MIN_CUTOFF;  // Current filter cutoff (Hz) - starts closed
volatile bool isTouching = false;  // Touch state - only generate audio when touching
volatile float phase = 0.0;  // Phase for sine wave
volatile float sawPhase = 0.0;  // Phase for sawtooth wave (one octave above)

// Biquad filter state (per channel)
struct BiquadState {
  float x1, x2;  // Input history
  float y1, y2;  // Output history
  float a0, a1, a2, b1, b2;  // Filter coefficients
};

BiquadState filterLeft = {0, 0, 0, 0, 0, 0, 0, 0, 0};
BiquadState filterRight = {0, 0, 0, 0, 0, 0, 0, 0, 0};

// Delay buffer (circular buffer for delay effect)
// Like a DAW send: delay continues even after touch ends
// Using int16_t to save memory (50% reduction): delayBuffer uses ~11.6KB instead of ~23KB
// Note: Filter is applied BEFORE sending to delay (instrument channel), not on delay return
int16_t delayBuffer[DELAY_BUFFER_SIZE] = {0};
int delayWriteIndex = 0;  // Write position in delay buffer (integer)

// Delay read positions (simple integer indices for circular buffer)
int delayReadIndexLeft = 0;   // Read position for left channel (delay - variance)
int delayReadIndexRight = 0;  // Read position for right channel (delay + variance)

// Sample rate conversion state (for delay running at different rate than audio)
int delayWriteCounter = 0;  // Counter for downsampling when writing to delay
int delayReadCounterLeft = 0;   // Counter for upsampling when reading from delay (left)
int delayReadCounterRight = 0;  // Counter for upsampling when reading from delay (right)
float delayReadLastLeft = 0.0;   // Last read value for interpolation (left)
float delayReadLastRight = 0.0;  // Last read value for interpolation (right)

// Filter states for instrument channel (applied before delay send)
// Delay return has no filter - it's just the delayed signal

// Display update timing
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 50; // Update display every 50ms for snappier response
unsigned long lastStaticRedraw = 0;
const unsigned long STATIC_REDRAW_INTERVAL = 2000; // Redraw static elements every 2 seconds

// Pentatonic scale intervals (frequency ratios)
// Pentatonic: Root, Minor 3rd, Perfect 4th, Perfect 5th, Minor 7th
const float PENTATONIC_RATIOS[5] = {1.0, 1.189, 1.335, 1.498, 1.782};
const int NUM_PENTATONIC_ZONES = 5;

// Touch state
bool touchDetected = false;
int touchX = 0;
int touchY = 0;
int currentPentatonicZone = -1;  // Current pentatonic zone being touched (-1 = none)

// FreeRTOS task handles
TaskHandle_t audioTaskHandle = NULL;

// Draw pentatonic zone dividers (yellow outlines)
void drawPentatonicZones() {
  // Calculate how many octaves fit in our frequency range
  float numOctaves = log(MAX_FREQ / MIN_FREQ) / log(2.0);
  
  // Draw vertical lines for each pentatonic zone boundary (yellow outlines)
  for (int octave = 0; octave <= (int)numOctaves; octave++) {
    for (int zone = 0; zone < NUM_PENTATONIC_ZONES; zone++) {
      // Calculate frequency for this zone
      float baseFreq = MIN_FREQ * pow(2.0, octave);
      float zoneFreq = baseFreq * PENTATONIC_RATIOS[zone];
      
      if (zoneFreq > MAX_FREQ) break;
      
      // Convert frequency to screen X position (logarithmic)
      float logMin = log10(MIN_FREQ);
      float logMax = log10(MAX_FREQ);
      float logFreq = log10(zoneFreq);
      float normalizedPos = (logFreq - logMin) / (logMax - logMin);
      int xPos = (int)(normalizedPos * tft.width());
      
      if (xPos >= 0 && xPos < tft.width()) {
        // Draw vertical line (yellow outline, only in the playable area, avoiding dead zones)
        tft.drawLine(xPos, DEAD_ZONE_SIZE, xPos, tft.height() - DEAD_ZONE_SIZE, TFT_YELLOW);
      }
    }
  }
}

// Quantize frequency to nearest pentatonic note
float quantizeToPentatonic(float freq) {
  // Find which octave we're in
  float octaveNum = log(freq / MIN_FREQ) / log(2.0);
  int octave = (int)octaveNum;
  float octaveFreq = MIN_FREQ * pow(2.0, octave);
  
  // Find which pentatonic interval within this octave
  float ratio = freq / octaveFreq;
  
  // Find closest pentatonic ratio
  int closestZone = 0;
  float minDiff = fabs(ratio - PENTATONIC_RATIOS[0]);
  
  for (int i = 1; i < NUM_PENTATONIC_ZONES; i++) {
    float diff = fabs(ratio - PENTATONIC_RATIOS[i]);
    if (diff < minDiff) {
      minDiff = diff;
      closestZone = i;
    }
  }
  
  // Also check next octave's root
  if (fabs(ratio - 2.0) < minDiff) {
    return octaveFreq * 2.0;  // Next octave root
  }
  
  // Return quantized frequency
  return octaveFreq * PENTATONIC_RATIOS[closestZone];
}

// Get pentatonic zone number for a given frequency
int getPentatonicZone(float freq) {
  // Find which octave we're in
  float octaveNum = log(freq / MIN_FREQ) / log(2.0);
  int octave = (int)octaveNum;
  float octaveFreq = MIN_FREQ * pow(2.0, octave);
  
  // Find which pentatonic interval within this octave
  float ratio = freq / octaveFreq;
  
  // Find closest pentatonic ratio
  int closestZone = 0;
  float minDiff = fabs(ratio - PENTATONIC_RATIOS[0]);
  
  for (int i = 1; i < NUM_PENTATONIC_ZONES; i++) {
    float diff = fabs(ratio - PENTATONIC_RATIOS[i]);
    if (diff < minDiff) {
      minDiff = diff;
      closestZone = i;
    }
  }
  
  // Also check next octave's root
  if (fabs(ratio - 2.0) < minDiff) {
    return 0;  // Next octave root
  }
  
  return closestZone;
}

// Calculate biquad low-pass filter coefficients with gain compensation
// This prevents the resonant peak from causing sudden volume jumps
void calculateBiquadCoefficients(BiquadState* filter, float cutoff, float q, float sampleRate, float gainComp) {
  float w0 = 2.0 * PI * cutoff / sampleRate;
  float cos_w0 = cos(w0);
  float sin_w0 = sin(w0);
  float alpha = sin_w0 / (2.0 * q);
  
  float b0 = (1.0 - cos_w0) / 2.0;
  float b1 = 1.0 - cos_w0;
  float b2 = (1.0 - cos_w0) / 2.0;
  float a0 = 1.0 + alpha;
  float a1 = -2.0 * cos_w0;
  float a2 = 1.0 - alpha;
  
  // Normalize coefficients and apply gain compensation
  // Gain compensation reduces the resonant peak's volume boost to prevent sudden jumps
  float norm = 1.0 / a0;
  filter->a0 = b0 * norm * gainComp;
  filter->a1 = b1 * norm * gainComp;
  filter->a2 = b2 * norm * gainComp;
  filter->b1 = a1 * norm;
  filter->b2 = a2 * norm;
}

// Apply biquad filter to a sample
float applyBiquad(BiquadState* filter, float input) {
  float output = filter->a0 * input + 
                 filter->a1 * filter->x1 + 
                 filter->a2 * filter->x2 -
                 filter->b1 * filter->y1 -
                 filter->b2 * filter->y2;
  
  // Update history
  filter->x2 = filter->x1;
  filter->x1 = input;
  filter->y2 = filter->y1;
  filter->y1 = output;
  
  return output;
}

// Audio generation task (runs on Core 0)
void audioTask(void *parameter) {
  // Buffer for stereo I2S: 64 samples * 2 channels = 128 samples
  int16_t audioBuffer[128];  // Interleaved L/R samples for stereo
  
  Serial2.println("Audio task started - generating sine + sawtooth with delay and filter");
  Serial2.printf("Delay: %.0fms, Feedback: %.0f%%, Stereo width: %.1fms, Send: %.0f%%\n", 
                 DELAY_TIME_MS, DELAY_FEEDBACK * 100.0, DELAY_TIME_VARIANCE_MS, DELAY_SEND_LEVEL * 100.0);
  Serial2.printf("Delay sample rate: %d Hz (Audio: %d Hz)\n", DELAY_SAMPLE_RATE, SAMPLE_RATE);
  Serial2.printf("Delay buffer size: %d samples (%.2f KB)\n", DELAY_BUFFER_SIZE, (DELAY_BUFFER_SIZE * 2) / 1024.0);
  
  // Calculate delay buffer read positions for stereo width
  // Note: These are calculated in delay sample rate, not audio sample rate
  // Left channel: delay time - variance (slightly shorter delay)
  // Right channel: delay time + variance (slightly longer delay)
  int delaySamples = (int)(DELAY_TIME_MS * DELAY_SAMPLE_RATE / 1000.0);
  int varianceSamples = (int)(DELAY_TIME_VARIANCE_MS * DELAY_SAMPLE_RATE / 1000.0);
  int delayLeftSamples = delaySamples - varianceSamples;   // Left: shorter delay
  int delayRightSamples = delaySamples + varianceSamples;  // Right: longer delay
  
  // Initialize read indices (will be updated as write index advances)
  delayReadIndexLeft = (DELAY_BUFFER_SIZE - delayLeftSamples) % DELAY_BUFFER_SIZE;
  delayReadIndexRight = (DELAY_BUFFER_SIZE - delayRightSamples) % DELAY_BUFFER_SIZE;
  
  // Initialize sample rate conversion counters
  delayWriteCounter = 0;
  delayReadCounterLeft = 0;
  delayReadCounterRight = 0;
  delayReadLastLeft = 0.0;
  delayReadLastRight = 0.0;
  
  unsigned long lastDebugTime = 0;
  uint32_t totalBytesWritten = 0;
  float lastCutoff = FILTER_MIN_CUTOFF;
  
  while (true) {
    // Update phase increment and filter cutoff (read volatile once per buffer)
    float freq = currentFreq;
    float cutoff = currentFilterCutoff;
    float phaseInc = 2.0 * PI * freq / SAMPLE_RATE;
    
    // Update filter coefficients if cutoff changed
    // Use smooth interpolation to prevent sudden changes
    if (fabs(cutoff - lastCutoff) > 1.0) {  // Only update if change is significant
      calculateBiquadCoefficients(&filterLeft, cutoff, FILTER_Q, SAMPLE_RATE, FILTER_GAIN_COMP);
      calculateBiquadCoefficients(&filterRight, cutoff, FILTER_Q, SAMPLE_RATE, FILTER_GAIN_COMP);
      lastCutoff = cutoff;
    }
    
    // Audio processing flow (mixing desk architecture):
    // 1. Generate sine + sawtooth waves (ONLY when touching) -> instrument signal
    // 2. Apply filter to instrument signal -> filtered instrument (instrument channel)
    // 3. Send filtered instrument to delay (send level) -> delay input
    // 4. Delay processes and outputs -> delay return (no filter on delay)
    // 5. Mix: filtered instrument (dry, only when touching) + delay return (always) -> master output
    // 6. Send to DAC (Left and Right channels)
    
    for (int i = 0; i < 64; i++) {
      float instrumentSignal = 0.0;  // Instrument signal (only when touching)
      
      // ============================================
      // STEP 1: Generate sine + sawtooth waves (ONLY when touching) -> instrument signal
      // ============================================
      if (isTouching) {
        // Generate sine wave at current frequency
        float sineSample = sin(phase);
        phase += phaseInc;
      if (phase >= 2.0 * PI) {
        phase -= 2.0 * PI;
        }
        
        // Generate sawtooth wave at one octave above (2x frequency)
        float sawFreq = freq * 2.0;
        float sawPhaseInc = 2.0 * PI * sawFreq / SAMPLE_RATE;
        float sawSample = (sawPhase / PI) - 1.0;  // Sawtooth: -1 to 1
        sawPhase += sawPhaseInc;
        if (sawPhase >= 2.0 * PI) {
          sawPhase -= 2.0 * PI;
        }
        
        // Mix both waves (equal mix) -> instrument signal
        instrumentSignal = (sineSample + sawSample) / 2.0;
      } else {
        // Not touching: reset phases to prevent phase jumps when touch resumes
        phase = 0.0;
        sawPhase = 0.0;
      }
      
      // ============================================
      // STEP 2: Apply filter to instrument signal -> filtered instrument (instrument channel)
      // ============================================
      float filteredInstrumentLeft = applyBiquad(&filterLeft, instrumentSignal);
      float filteredInstrumentRight = applyBiquad(&filterRight, instrumentSignal);
      
      // ============================================
      // STEP 3: Send filtered instrument to delay (send level) -> delay input
      // ============================================
      // Read delayed signals from buffer (with stereo width) - delay return (no filter)
      // Handle sample rate conversion if delay runs at different rate than audio
      float delayReturnLeft, delayReturnRight;
      
      if (DELAY_SAMPLE_RATE == SAMPLE_RATE) {
        // Same sample rate: direct read (convert int16 to float)
        delayReturnLeft = (float)delayBuffer[delayReadIndexLeft] / 32768.0;
        delayReturnRight = (float)delayBuffer[delayReadIndexRight] / 32768.0;
        
        // Advance read indices (circular buffer)
        delayReadIndexLeft = (delayReadIndexLeft + 1) % DELAY_BUFFER_SIZE;
        delayReadIndexRight = (delayReadIndexRight + 1) % DELAY_BUFFER_SIZE;
      } else {
        // Different sample rates: need upsampling (delay is lower rate)
        float sampleRateRatio = (float)SAMPLE_RATE / DELAY_SAMPLE_RATE;
        
        // Left channel upsampling
        if (delayReadCounterLeft >= (int)sampleRateRatio) {
          delayReadCounterLeft = 0;
          delayReadLastLeft = (float)delayBuffer[delayReadIndexLeft] / 32768.0;
          delayReadIndexLeft = (delayReadIndexLeft + 1) % DELAY_BUFFER_SIZE;
        }
        delayReturnLeft = delayReadLastLeft;  // Simple hold (can use linear interpolation for better quality)
        delayReadCounterLeft++;
        
        // Right channel upsampling
        if (delayReadCounterRight >= (int)sampleRateRatio) {
          delayReadCounterRight = 0;
          delayReadLastRight = (float)delayBuffer[delayReadIndexRight] / 32768.0;
          delayReadIndexRight = (delayReadIndexRight + 1) % DELAY_BUFFER_SIZE;
        }
        delayReturnRight = delayReadLastRight;  // Simple hold
        delayReadCounterRight++;
      }
      
      // Mix filtered instrument with feedback from delay (feedback continues even when not touching)
      // Use average of left and right for mono feedback
      float delayFeedback = (delayReturnLeft + delayReturnRight) / 2.0;
      float delayInput = (filteredInstrumentLeft * DELAY_SEND_LEVEL) + (delayFeedback * DELAY_FEEDBACK);
      
      // Write to delay buffer (downsample if delay runs at lower rate)
      if (DELAY_SAMPLE_RATE == SAMPLE_RATE) {
        // Same sample rate: write every sample
        // Convert float to int16_t and store
        int16_t delaySample = (int16_t)(constrain(delayInput * 32768.0, -32768.0, 32767.0));
        delayBuffer[delayWriteIndex] = delaySample;
        delayWriteIndex = (delayWriteIndex + 1) % DELAY_BUFFER_SIZE;
      } else {
        // Different sample rates: downsample (write only every Nth sample)
        float sampleRateRatio = (float)SAMPLE_RATE / DELAY_SAMPLE_RATE;
        delayWriteCounter++;
        
        if (delayWriteCounter >= (int)sampleRateRatio) {
          delayWriteCounter = 0;
          // Convert float to int16_t and store
          int16_t delaySample = (int16_t)(constrain(delayInput * 32768.0, -32768.0, 32767.0));
          delayBuffer[delayWriteIndex] = delaySample;
          delayWriteIndex = (delayWriteIndex + 1) % DELAY_BUFFER_SIZE;
        }
      }
      
      // ============================================
      // STEP 4: Delay return (no filter on delay - it's already filtered from input)
      // ============================================
      // Delay return is already read above (delayReturnLeft, delayReturnRight)
      // No filtering needed - delay just echoes what was sent to it
      
      // ============================================
      // STEP 5: Mix filtered instrument (dry, only when touching) + delay return (always) -> master output
      // ============================================
      float masterLeft = filteredInstrumentLeft + delayReturnLeft;
      float masterRight = filteredInstrumentRight + delayReturnRight;
      
      // ============================================
      // STEP 6: Hard limit/clip to protect DAC and user's ears
      // ============================================
      // Hard clip to -1.0 to 1.0 range (safety limit before DAC)
      // This prevents any possibility of clipping or damage
      masterLeft = constrain(masterLeft, -1.0, 1.0);
      masterRight = constrain(masterRight, -1.0, 1.0);
      
      // ============================================
      // STEP 7: Scale to fixed amplitude and convert to int16
      // ============================================
      int16_t leftSample = (int16_t)(masterLeft * FIXED_AMPLITUDE);
      int16_t rightSample = (int16_t)(masterRight * FIXED_AMPLITUDE);
      
      // ============================================
      // STEP 8: Send to DAC (interleaved Left and Right)
      // ============================================
      audioBuffer[i * 2] = leftSample;      // Left channel
      audioBuffer[i * 2 + 1] = rightSample; // Right channel
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
  // Draw dead zones on screen
  tft.drawRect(0, 0, tft.width(), DEAD_ZONE_SIZE, TFT_RED);
  tft.drawRect(0, tft.height() - DEAD_ZONE_SIZE, tft.width(), DEAD_ZONE_SIZE, TFT_RED);
  
  // Draw pentatonic zone dividers (will be redrawn periodically)
  drawPentatonicZones();
  
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
  
  // Initialize filter coefficients (starts closed at 80Hz)
  calculateBiquadCoefficients(&filterLeft, FILTER_MIN_CUTOFF, FILTER_Q, SAMPLE_RATE, FILTER_GAIN_COMP);
  calculateBiquadCoefficients(&filterRight, FILTER_MIN_CUTOFF, FILTER_Q, SAMPLE_RATE, FILTER_GAIN_COMP);
  Serial2.printf("Low-pass filter initialized: Cutoff=%.0f Hz, Q=%.3f, GainComp=%.2f\n", FILTER_MIN_CUTOFF, FILTER_Q, FILTER_GAIN_COMP);
  
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
  // Touch polling is done as fast as possible for smooth audio response
  // Display updates are separate and less frequent
  
  // Check for touch input - poll continuously for smooth response
  if (ts.tirqTouched() && ts.touched()) {
    TS_Point p = ts.getPoint();
    
    // Convert touch coordinates to display coordinates
    // Calibration values from Random Nerd Tutorials example for CYD board
    touchX = map(p.x, 200, 3700, 1, tft.width());
    touchY = map(p.y, 240, 3800, 1, tft.height());
    
    // Constrain to display bounds
    touchX = constrain(touchX, 0, tft.width() - 1);
    touchY = constrain(touchY, 0, tft.height() - 1);
    
    // X-axis controls pitch: 80Hz (left) to 2kHz (right) - quantized to pentatonic scale
    // Use logarithmic scaling for natural pitch perception (like musical intervals)
    // Map linear touch position to logarithmic frequency range
    float xPos = map(touchX, 0, tft.width() - 1, 0, 100);
    xPos = constrain(xPos, 0, 100);
    
    // Logarithmic scaling: convert linear 0-100 to logarithmic frequency
    float logMin = log10(MIN_FREQ);
    float logMax = log10(MAX_FREQ);
    float logRange = logMax - logMin;
    
    // Map linear position to logarithmic frequency
    float logFreq = logMin + (xPos / 100.0) * logRange;
    float rawFreq = pow(10.0, logFreq);
    rawFreq = constrain(rawFreq, MIN_FREQ, MAX_FREQ);
    
    // Quantize to nearest pentatonic note
    float quantizedFreq = quantizeToPentatonic(rawFreq);
    quantizedFreq = constrain(quantizedFreq, MIN_FREQ, MAX_FREQ);
    currentFreq = quantizedFreq;
    
    // Determine which pentatonic zone we're in (use quantized frequency)
    currentPentatonicZone = getPentatonicZone(quantizedFreq);
    
    // Y-axis controls low-pass filter cutoff with dead zones and logarithmic scaling
    // Dead zones: top DEAD_ZONE_SIZE pixels = filter open (2kHz+), bottom DEAD_ZONE_SIZE pixels = filter closed (<80Hz)
    // Middle area uses logarithmic scaling for natural filter perception
    float filterCutoff;
    
    if (touchY < DEAD_ZONE_SIZE) {
      // Top dead zone: filter open (2kHz+) - doesn't affect sound
      filterCutoff = FILTER_MAX_CUTOFF;
    } else if (touchY > tft.height() - DEAD_ZONE_SIZE) {
      // Bottom dead zone: filter closed (<80Hz) - completely kills sound
      filterCutoff = FILTER_MIN_CUTOFF;
    } else {
      // Middle area: logarithmic scaling from 80Hz (bottom) to 2kHz (top)
      // Map Y from dead zone boundaries to 0.0-1.0 (normalized position)
      // Note: touchY increases downward, so bottom of middle has higher touchY value
      float middleHeight = tft.height() - (2 * DEAD_ZONE_SIZE);
      float yPosInMiddle = touchY - DEAD_ZONE_SIZE;  // Position within middle area
      // Normalize: 0.0 at bottom of middle (high touchY), 1.0 at top of middle (low touchY)
      float normalizedPos = yPosInMiddle / middleHeight;  // 0.0 (bottom) to 1.0 (top)
      normalizedPos = constrain(normalizedPos, 0.0, 1.0);
      
      // Invert: bottom of middle (normalizedPos=0, high touchY) should be closed (80Hz)
      //         top of middle (normalizedPos=1, low touchY) should be open (2kHz)
      float invertedPos = 1.0 - normalizedPos;  // 1.0 at bottom, 0.0 at top
      
      // Logarithmic scaling: map 0.0-1.0 to filter cutoff range (human perception)
      // Minimum cutoff: 80Hz (filter closed) at bottom
      // Maximum cutoff: 2000Hz (filter open) at top
      float logMin = log10(FILTER_MIN_CUTOFF);
      float logMax = log10(FILTER_MAX_CUTOFF);
      float logRange = logMax - logMin;
      
      // Convert: invertedPos=1.0 (bottom) -> logCutoff=logMin (80Hz), invertedPos=0.0 (top) -> logCutoff=logMax (2kHz)
      float logCutoff = logMin + invertedPos * logRange;
      filterCutoff = pow(10.0, logCutoff);
    }
    
    filterCutoff = constrain(filterCutoff, FILTER_MIN_CUTOFF, FILTER_MAX_CUTOFF);
    currentFilterCutoff = filterCutoff;
    touchDetected = true;
    isTouching = true;  // Set touch flag for audio task (only generate audio when touching)
    
  } else {
    // No touch - keep filter at last position (sound continues)
    touchDetected = false;
    isTouching = false;  // Clear touch flag (stop generating new audio, but delay continues)
    currentPentatonicZone = -1;
  }
  
  // Update display periodically (separate from touch polling for performance)
  unsigned long currentTime = millis();
  
  // Redraw static elements periodically to prevent "rubbing out"
  if (currentTime - lastStaticRedraw >= STATIC_REDRAW_INTERVAL) {
    lastStaticRedraw = currentTime;
    
    // Redraw dead zones
    tft.drawRect(0, 0, tft.width(), DEAD_ZONE_SIZE, TFT_RED);
    tft.drawRect(0, tft.height() - DEAD_ZONE_SIZE, tft.width(), DEAD_ZONE_SIZE, TFT_RED);
    
    // Redraw pentatonic zone dividers
    drawPentatonicZones();
  }
  
  if (currentTime - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    lastDisplayUpdate = currentTime;
    
    // Highlight current pentatonic zone (filled yellow when playing, yellow outline when not)
    static int lastXStart = -1, lastXEnd = -1;
    static int lastZone = -1;
    
    if (touchDetected && currentPentatonicZone >= 0) {
      // Calculate zone boundaries for the quantized frequency
      float freq = currentFreq;
      float octaveNum = log(freq / MIN_FREQ) / log(2.0);
      int octave = (int)octaveNum;
      float octaveFreq = MIN_FREQ * pow(2.0, octave);
      
      // Find the exact zone boundaries for the quantized note
      float zoneStartFreq = octaveFreq * PENTATONIC_RATIOS[currentPentatonicZone];
      float zoneEndFreq;
      
      // Find the next pentatonic note (could be in same octave or next)
      if (currentPentatonicZone < NUM_PENTATONIC_ZONES - 1) {
        zoneEndFreq = octaveFreq * PENTATONIC_RATIOS[currentPentatonicZone + 1];
      } else {
        // Last note in octave, next is root of next octave
        zoneEndFreq = octaveFreq * 2.0;
      }
      
      // Ensure boundaries are within our range
      zoneStartFreq = constrain(zoneStartFreq, MIN_FREQ, MAX_FREQ);
      zoneEndFreq = constrain(zoneEndFreq, MIN_FREQ, MAX_FREQ);
      
      // Convert to screen positions (logarithmic)
      float logMin = log10(MIN_FREQ);
      float logMax = log10(MAX_FREQ);
      float logStart = log10(zoneStartFreq);
      float logEnd = log10(zoneEndFreq);
      int xStart = (int)(((logStart - logMin) / (logMax - logMin)) * tft.width());
      int xEnd = (int)(((logEnd - logMin) / (logMax - logMin)) * tft.width());
      xStart = constrain(xStart, 0, tft.width() - 1);
      xEnd = constrain(xEnd, 0, tft.width() - 1);
      
      // Ensure xEnd >= xStart
      if (xEnd < xStart) {
        int temp = xStart;
        xStart = xEnd;
        xEnd = temp;
      }
      
      // Only update if zone changed
      if (lastXStart != xStart || lastXEnd != xEnd || lastZone != currentPentatonicZone) {
        // Erase old highlight: fill with black and redraw yellow outline
        if (lastXStart >= 0 && lastXEnd >= 0) {
          tft.fillRect(lastXStart + 1, DEAD_ZONE_SIZE, lastXEnd - lastXStart - 1, 
                      tft.height() - (2 * DEAD_ZONE_SIZE), TFT_BLACK);
          // Redraw yellow outline on left edge
          if (lastXStart > 0) {
            tft.drawLine(lastXStart, DEAD_ZONE_SIZE, lastXStart, tft.height() - DEAD_ZONE_SIZE, TFT_YELLOW);
          }
          // Redraw yellow outline on right edge
          if (lastXEnd < tft.width() - 1) {
            tft.drawLine(lastXEnd, DEAD_ZONE_SIZE, lastXEnd, tft.height() - DEAD_ZONE_SIZE, TFT_YELLOW);
          }
        }
        
        // Draw new highlight: fill with yellow
        if (xEnd > xStart) {
          tft.fillRect(xStart + 1, DEAD_ZONE_SIZE, xEnd - xStart - 1, 
                      tft.height() - (2 * DEAD_ZONE_SIZE), TFT_YELLOW);
        }
        
        lastXStart = xStart;
        lastXEnd = xEnd;
        lastZone = currentPentatonicZone;
      }
    } else {
      // Not touching - erase highlight and restore yellow outline
      if (lastXStart >= 0 && lastXEnd >= 0) {
        // Fill with black
        tft.fillRect(lastXStart + 1, DEAD_ZONE_SIZE, lastXEnd - lastXStart - 1, 
                    tft.height() - (2 * DEAD_ZONE_SIZE), TFT_BLACK);
        // Redraw yellow outline on left edge
        if (lastXStart > 0) {
          tft.drawLine(lastXStart, DEAD_ZONE_SIZE, lastXStart, tft.height() - DEAD_ZONE_SIZE, TFT_YELLOW);
        }
        // Redraw yellow outline on right edge
        if (lastXEnd < tft.width() - 1) {
          tft.drawLine(lastXEnd, DEAD_ZONE_SIZE, lastXEnd, tft.height() - DEAD_ZONE_SIZE, TFT_YELLOW);
        }
        lastXStart = -1;
        lastXEnd = -1;
        lastZone = -1;
      }
    }
  }
  
  // Minimal delay - touch polling is priority for smooth audio
  // Use yield() instead of delay() to allow other tasks to run
  yield();
}

