/*
 * Scale Abstraction Implementation
 */

#include "scale.h"
#include "../config.h"
#include <math.h>

// Pentatonic scale ratios (normalized to root = 1.0)
const float PentatonicScale::RATIOS[5] = {
  1.0,      // Root
  1.189,    // Minor 3rd
  1.335,    // Perfect 4th
  1.498,    // Perfect 5th
  1.782     // Minor 7th
};

PentatonicScale::PentatonicScale() : currentBaseFreq(MIN_FREQ_BASE) {
}

void PentatonicScale::setBaseFrequency(float baseFreq) {
  currentBaseFreq = baseFreq;
}

float PentatonicScale::quantize(float frequency) {
  // Find which octave we're in (using current base frequency)
  float octaveNum = log(frequency / currentBaseFreq) / log(2.0);
  int octave = (int)octaveNum;
  float octaveFreq = currentBaseFreq * pow(2.0, octave);
  
  // Find which pentatonic interval within this octave
  float ratio = frequency / octaveFreq;
  
  // Find closest pentatonic ratio
  int closestZone = 0;
  float minDiff = fabs(ratio - RATIOS[0]);
  
  for (int i = 1; i < NUM_RATIOS; i++) {
    float diff = fabs(ratio - RATIOS[i]);
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
  return octaveFreq * RATIOS[closestZone];
}

void PentatonicScale::getRatios(float* ratios, int& numRatios) {
  for (int i = 0; i < NUM_RATIOS; i++) {
    ratios[i] = RATIOS[i];
  }
  numRatios = NUM_RATIOS;
}

// Chromatic scale ratios (12 semitones per octave)
const float ChromaticScale::RATIOS[12] = {
  1.0,                    // C (root)
  1.0594630943592953,     // C# (2^(1/12))
  1.122462048309373,      // D (2^(2/12))
  1.189207115002721,      // D# (2^(3/12))
  1.2599210498948732,     // E (2^(4/12))
  1.3348398541700344,     // F (2^(5/12))
  1.4142135623730951,     // F# (2^(6/12))
  1.4983070768766815,     // G (2^(7/12))
  1.5874010519681994,     // G# (2^(8/12))
  1.681792830507429,      // A (2^(9/12))
  1.7817974362806785,     // A# (2^(10/12))
  1.8877486253633868      // B (2^(11/12))
};

ChromaticScale::ChromaticScale() : currentBaseFreq(MIN_FREQ_BASE) {
}

void ChromaticScale::setBaseFrequency(float baseFreq) {
  currentBaseFreq = baseFreq;
}

float ChromaticScale::quantize(float frequency) {
  // Find which octave we're in
  float octaveNum = log(frequency / currentBaseFreq) / log(2.0);
  int octave = (int)octaveNum;
  float octaveFreq = currentBaseFreq * pow(2.0, octave);
  
  // Find which semitone within this octave
  float ratio = frequency / octaveFreq;
  
  // Find closest semitone
  int closestSemitone = 0;
  float minDiff = fabs(ratio - RATIOS[0]);
  
  for (int i = 1; i < NUM_RATIOS; i++) {
    float diff = fabs(ratio - RATIOS[i]);
    if (diff < minDiff) {
      minDiff = diff;
      closestSemitone = i;
    }
  }
  
  // Also check next octave's root
  if (fabs(ratio - 2.0) < minDiff) {
    return octaveFreq * 2.0;  // Next octave root
  }
  
  // Return quantized frequency
  return octaveFreq * RATIOS[closestSemitone];
}

void ChromaticScale::getRatios(float* ratios, int& numRatios) {
  for (int i = 0; i < NUM_RATIOS; i++) {
    ratios[i] = RATIOS[i];
  }
  numRatios = NUM_RATIOS;
}

// Factory function
Scale* createScale(ScaleType type) {
  switch (type) {
    case SCALE_PENTATONIC:
      return new PentatonicScale();
    case SCALE_CHROMATIC:
      return new ChromaticScale();
    default:
      return new PentatonicScale();  // Default to pentatonic
  }
}

