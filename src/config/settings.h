/*
 * Settings Structure
 * Configuration for different modes and options
 * This structure will be expanded as features are added
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>

// Scale types
enum ScaleType {
  SCALE_PENTATONIC,    // 5 notes per octave (current default)
  SCALE_CHROMATIC,     // 12 notes per octave
  SCALE_MAJOR,         // 7 notes per octave (major scale)
  SCALE_MINOR,         // 7 notes per octave (minor scale)
  SCALE_DORIAN,        // 7 notes per octave (dorian mode)
  SCALE_MIXOLYDIAN,    // 7 notes per octave (mixolydian mode)
  NUM_SCALE_TYPES
};

// Pitch modes
enum PitchMode {
  PITCH_MODE_QUANTIZED,   // Quantized to scale (current default)
  PITCH_MODE_CONTINUOUS   // Continuous pitch (no quantization)
};

// Waveshape types
enum WaveshapeType {
  WAVESHAPE_SINE,          // Sine wave
  WAVESHAPE_SAWTOOTH,      // Sawtooth wave
  WAVESHAPE_SQUARE,        // Square wave
  WAVESHAPE_TRIANGLE,      // Triangle wave
  WAVESHAPE_SINE_SAW,      // Sine + Sawtooth mix (current default)
  NUM_WAVESHAPE_TYPES
};

// Effect types (bit flags for multiple effects)
#define EFFECT_NONE        0x00
#define EFFECT_DELAY       0x01
#define EFFECT_DISTORTION  0x02
#define EFFECT_REVERB      0x04
#define EFFECT_CHORUS      0x08

// Parameter visibility flags (bit flags)
#define PARAM_VISIBLE_DELAY_TIME    0x01
#define PARAM_VISIBLE_LFO_DEPTH     0x02
#define PARAM_VISIBLE_LFO_SPEED      0x04
#define PARAM_VISIBLE_BASE_NOTE     0x08
#define PARAM_VISIBLE_UPPER_TONE    0x10
#define PARAM_VISIBLE_MASTER_GAIN   0x20
#define PARAM_VISIBLE_DELAY_SEND    0x40
#define PARAM_VISIBLE_FILTER_Q      0x80

// Settings structure
struct Settings {
  // Scale and pitch settings
  ScaleType scaleType;           // Which scale to use
  int numOctaves;                // Number of octaves on keyboard (1-4)
  PitchMode pitchMode;            // Quantized or continuous
  
  // Oscillator settings
  WaveshapeType primaryWaveshape; // Primary oscillator waveshape
  WaveshapeType secondaryWaveshape; // Secondary oscillator waveshape (if dual oscillator)
  float waveshapeMix;            // Mix between primary and secondary (0.0-1.0)
  
  // Effect settings
  uint8_t activeEffects;         // Bit flags for active effects
  
  // Parameter visibility
  uint8_t visibleParameters;    // Bit flags for which parameters are visible
  
  // Default constructor
  Settings() {
    scaleType = SCALE_PENTATONIC;
    numOctaves = 2;
    pitchMode = PITCH_MODE_QUANTIZED;
    primaryWaveshape = WAVESHAPE_SINE;
    secondaryWaveshape = WAVESHAPE_SAWTOOTH;
    waveshapeMix = 0.5;  // 50/50 mix
    activeEffects = EFFECT_DELAY;  // Only delay active by default
    visibleParameters = PARAM_VISIBLE_DELAY_TIME | PARAM_VISIBLE_LFO_DEPTH | 
                       PARAM_VISIBLE_LFO_SPEED | PARAM_VISIBLE_BASE_NOTE | 
                       PARAM_VISIBLE_UPPER_TONE;  // Current 5 parameters
  }
};

#endif // SETTINGS_H

