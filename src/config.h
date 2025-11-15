/*
 * Configuration constants for CYD Synthesizer
 * Centralized configuration for easy tuning
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <math.h>
#ifndef PI
#define PI 3.14159265358979323846
#endif

// Audio parameters
#define SAMPLE_RATE     44100
#define MIN_FREQ        80.0   // Minimum frequency (Hz) for pitch control - pentatonic scale
#define MAX_FREQ        2000.0 // Maximum frequency (Hz) for pitch control - pentatonic scale
#define MAX_AMPLITUDE   32767   // Max amplitude for 16-bit (32767 max)
#define FIXED_VOLUME_PERCENT 10.0  // Fixed volume percentage
#define FIXED_AMPLITUDE ((FIXED_VOLUME_PERCENT / 100.0) * MAX_AMPLITUDE)

// Delay parameters
#define DELAY_TIME_MS    250.0  // Delay time in milliseconds (reduced from 400ms for memory)
#define DELAY_FEEDBACK   0.9    // Feedback level (90%)
#define DELAY_TIME_VARIANCE_MS 25.0  // Stereo width: time difference between L and R channels
#define DELAY_SEND_LEVEL 0.7    // Send level: how much of the filtered instrument goes to delay (0.0 to 1.0)

// Delay sample rate: can be set to 22050 for lower memory usage (A/B test)
// Set to SAMPLE_RATE for full quality, or 22050 for half memory usage
#define DELAY_SAMPLE_RATE 22050  // Change to SAMPLE_RATE to use full quality delay
// #define DELAY_SAMPLE_RATE SAMPLE_RATE  // Uncomment this and comment above for full quality

#define DELAY_BUFFER_SIZE ((int)((DELAY_TIME_MS + DELAY_TIME_VARIANCE_MS) * DELAY_SAMPLE_RATE / 1000.0) + 1)  // Buffer size in samples

// Filter parameters
#define FILTER_MIN_CUTOFF  40.0   // Minimum filter cutoff (Hz) - below this kills sound completely
#define FILTER_MAX_CUTOFF  4000.0  // Maximum filter cutoff (Hz) - above this doesn't affect sound
#define FILTER_Q           1.5     // Filter Q (resonance) - reduced to avoid sudden volume jumps
#define FILTER_GAIN_COMP   0.7     // Gain compensation to prevent resonance from causing volume jumps

// Display parameters
#define DEAD_ZONE_SIZE  20  // Pixels from top/bottom for dead zones
#define DISPLAY_UPDATE_INTERVAL 50  // Update display every 50ms for snappier response
#define STATIC_REDRAW_INTERVAL 2000  // Redraw static elements every 2 seconds

// Pentatonic scale intervals (frequency ratios)
// Pentatonic: Root, Minor 3rd, Perfect 4th, Perfect 5th, Minor 7th
#define NUM_PENTATONIC_ZONES 5
const float PENTATONIC_RATIOS[5] = {1.0, 1.189, 1.335, 1.498, 1.782};

// I2S Pin Configuration
#define I2S_NUM         I2S_NUM_0
#define I2S_BCK_PIN     1   // Bit Clock - IO1 (TX) from P1 connector (YellowBlack wire)
#define I2S_LRCK_PIN    27  // Word Select / Left-Right Clock - IO27 from CN1 connector (Yellow wire)
#define I2S_DATA_PIN    22  // Data Input - IO22 from CN1 connector (Blue wire)

// Touchscreen pins for CYD board (ESP32-2432S028R)
#define XPT2046_IRQ 36   // T_IRQ pin
#define XPT2046_MOSI 32  // T_DIN pin
#define XPT2046_MISO 39  // T_OUT pin
#define XPT2046_CLK 25   // T_CLK pin
#define XPT2046_CS 33    // T_CS pin

#endif // CONFIG_H

