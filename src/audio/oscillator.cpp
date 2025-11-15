/*
 * Dual Oscillator Implementation
 */

#include "oscillator.h"
#include "../config.h"
#include <math.h>

Oscillator::Oscillator() : phase(0.0), sawPhase(0.0), semitoneRatio(2.0) {
  // Initialize with 12 semitones (one octave) = 2.0 ratio
}

void Oscillator::setSemitoneInterval(float semitones) {
  // Calculate semitone ratio once when interval changes (not in audio loop!)
  // Semitone ratio = 2^(semitones/12)
  semitoneRatio = pow(2.0, semitones / 12.0);
}

float Oscillator::generate(float frequency, bool active) {
  // Always generate signal (envelope will control amplitude)
  // Don't reset phases when inactive - let envelope handle smooth transitions
  // This keeps phases continuous and prevents clicks/pops
  
  // Generate sine wave at current frequency
  float sineSample = sin(phase);
  float phaseInc = 2.0 * PI * frequency / SAMPLE_RATE;
  phase += phaseInc;
  if (phase >= 2.0 * PI) {
    phase -= 2.0 * PI;
  }
  
  // Generate sawtooth wave at specified semitone interval above sine
  // Use pre-calculated ratio (no pow() in audio loop!)
  float sawFreq = frequency * semitoneRatio;
  float sawPhaseInc = 2.0 * PI * sawFreq / SAMPLE_RATE;
  float sawSample = (sawPhase / PI) - 1.0;  // Sawtooth: -1 to 1
  sawPhase += sawPhaseInc;
  if (sawPhase >= 2.0 * PI) {
    sawPhase -= 2.0 * PI;
  }
  
  // Mix both waves (equal mix) -> instrument signal
  return (sineSample + sawSample) / 2.0;
}

