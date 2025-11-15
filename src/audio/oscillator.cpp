/*
 * Dual Oscillator Implementation
 */

#include "oscillator.h"
#include "../config.h"
#include <math.h>

Oscillator::Oscillator() : phase(0.0), sawPhase(0.0) {
}

float Oscillator::generate(float frequency, bool active) {
  if (!active) {
    // Not active: reset phases to prevent phase jumps when resuming
    phase = 0.0;
    sawPhase = 0.0;
    return 0.0;
  }
  
  // Generate sine wave at current frequency
  float sineSample = sin(phase);
  float phaseInc = 2.0 * PI * frequency / SAMPLE_RATE;
  phase += phaseInc;
  if (phase >= 2.0 * PI) {
    phase -= 2.0 * PI;
  }
  
  // Generate sawtooth wave at one octave above (2x frequency)
  float sawFreq = frequency * 2.0;
  float sawPhaseInc = 2.0 * PI * sawFreq / SAMPLE_RATE;
  float sawSample = (sawPhase / PI) - 1.0;  // Sawtooth: -1 to 1
  sawPhase += sawPhaseInc;
  if (sawPhase >= 2.0 * PI) {
    sawPhase -= 2.0 * PI;
  }
  
  // Mix both waves (equal mix) -> instrument signal
  return (sineSample + sawSample) / 2.0;
}

