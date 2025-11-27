/*
 * Configuration constants for CYD Synthesizer
 * Centralized configuration for easy tuning
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <math.h>
#ifdef PI
#undef PI
#endif
#define PI 3.14159265358979323846

// Audio parameters
#define SAMPLE_RATE     44100
#define MIN_FREQ_BASE   80.0   // Base minimum frequency (Hz) - around Eb2, will be adjusted by base note slider
#define MIN_FREQ        80.0   // Minimum frequency (Hz) for pitch control - pentatonic scale (runtime adjustable)
#define MAX_FREQ        2000.0 // Maximum frequency (Hz) for pitch control - pentatonic scale

// Base note slider: 5 detents, whole tone steps
// Middle = Eb2 (~80Hz), going down: Db2, B1, going up: F2, G2
#define BASE_NOTE_DETENTS 5
#define BASE_NOTE_DEFAULT 2  // Middle detent (0-4, 2 = Eb2)
// Whole tone = 2 semitones = 2^(2/12) frequency ratio
#define WHOLE_TONE_RATIO 1.122462048  // 2^(2/12)

// Upper tone slider: 5 detents for sawtooth interval
#define UPPER_TONE_DETENTS 5
#define UPPER_TONE_DEFAULT 3  // Middle detent (0-4, 3 = 12 semitones/octave)
// Semitone intervals: 5, 7, 9, 12, 14 semitones
const float UPPER_TONE_SEMITONES[5] = {5.0, 7.0, 9.0, 12.0, 14.0};
#define MAX_AMPLITUDE   32767   // Max amplitude for 16-bit (32767 max)
#define FIXED_VOLUME_PERCENT 10.0  // Fixed volume percentage
#define FIXED_AMPLITUDE ((FIXED_VOLUME_PERCENT / 100.0) * MAX_AMPLITUDE)

// Delay parameters
#define DELAY_TIME_MS_DEFAULT 300.0  // Default delay time in milliseconds
#define DELAY_TIME_MS_MIN 50.0       // Minimum delay time (ms)
#define DELAY_TIME_MS_MAX 1000.0     // Maximum delay time (ms)
#define DELAY_TIME_VARIANCE_MS_MIN 20.0  // Minimum stereo variance (ms)
#define DELAY_TIME_VARIANCE_MS_MAX 200.0 // Maximum stereo variance (ms)
#define DELAY_FEEDBACK   0.6    // Feedback level (60%)
#define DELAY_FEEDBACK_MIN 0.001  // Minimum feedback (0.1%)
#define DELAY_FEEDBACK_MAX 1.2   // Maximum feedback (120% - overdrive/runaway zone)
#define DELAY_TIME_VARIANCE_MS 45.0  // Stereo width: time difference between L and R channels
#define DELAY_SEND_LEVEL 0.3    // Send level: how much of the filtered instrument goes to delay (0.0 to 1.0)

// LFO parameters for delay time modulation
#define LFO_DEPTH_MS_MIN 0.0    // Minimum LFO depth (ms) - no modulation
#define LFO_DEPTH_MS_MAX 100.0  // Maximum LFO depth (ms) - adds/removes up to 100ms
#define LFO_DEPTH_MS_DEFAULT 0.0  // Default LFO depth (off)
#define LFO_SPEED_HZ_MIN 0.1    // Minimum LFO speed (Hz)
#define LFO_SPEED_HZ_MAX 50.0   // Maximum LFO speed (Hz)
#define LFO_SPEED_HZ_DEFAULT 1.0  // Default LFO speed (1 Hz)

// Delay sample rate: can be set to 22050 for lower memory usage (A/B test)
// Set to SAMPLE_RATE for full quality, or 22050 for half memory usage
#define DELAY_SAMPLE_RATE 22050  // Change to SAMPLE_RATE to use full quality delay
// #define DELAY_SAMPLE_RATE SAMPLE_RATE  // Uncomment this and comment above for full quality

// Calculate maximum buffer size needed (for max delay time + max variance)
#define DELAY_BUFFER_SIZE_MAX ((int)((DELAY_TIME_MS_MAX + DELAY_TIME_VARIANCE_MS_MAX) * DELAY_SAMPLE_RATE / 1000.0) + 1)  // Max buffer size in samples
#define DELAY_BUFFER_SIZE DELAY_BUFFER_SIZE_MAX  // Use max size to allow real-time delay changes

// Filter parameters
#define FILTER_MIN_CUTOFF  40.0   // Minimum filter cutoff (Hz) - below this kills sound completely
#define FILTER_MAX_CUTOFF  4000.0  // Maximum filter cutoff (Hz) - above this doesn't affect sound
#define FILTER_Q           1.5     // Filter Q (resonance) - reduced to avoid sudden volume jumps
#define FILTER_GAIN_COMP   0.7     // Gain compensation to prevent resonance from causing volume jumps

// Master output parameters
#define MASTER_GAIN 0.7  // Master gain reduction to prevent excessive peaks (0.0 to 1.0)
// Lower value = more headroom, less clipping. Higher value = louder but more clipping risk.
// 0.7 provides good headroom while maintaining reasonable volume

// Display parameters
#define DEAD_ZONE_SIZE  20  // Pixels from top/bottom for dead zones
#define DISPLAY_UPDATE_INTERVAL 50  // Update display every 50ms for snappier response
#define STATIC_REDRAW_INTERVAL 200  // Redraw static elements every 2 seconds
#define MENU_BUTTON_WIDTH 24  // Width of vertical menu button strip at bottom left (tappable area)
// Fixed per-fader strip width for bottom controls (compact layout, leaves empty space on right)
#define FADER_STRIP_WIDTH 26

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

