/*
 * LFO Implementation
 */

#include "lfo.h"
#include "../config.h"
#include <math.h>

LFO::LFO() : phase(0.0), phaseInc(0.0) {
}

void LFO::setSpeed(float frequencyHz) {
  // Calculate phase increment per sample
  phaseInc = 2.0 * PI * frequencyHz / SAMPLE_RATE;
}

float LFO::process() {
  // Generate sine wave (-1.0 to 1.0)
  float output = sin(phase);
  
  // Advance phase
  phase += phaseInc;
  if (phase >= 2.0 * PI) {
    phase -= 2.0 * PI;
  }
  
  return output;
}

