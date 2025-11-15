/*
 * Biquad Low-Pass Filter Implementation
 */

#include "filter.h"
#include "../config.h"
#include <math.h>

BiquadFilter::BiquadFilter() : lastCutoff(FILTER_MIN_CUTOFF) {
  // Initialize state
  state = {0, 0, 0, 0, 0, 0, 0, 0, 0};
  calculateCoefficients(FILTER_MIN_CUTOFF);
}

void BiquadFilter::setCutoff(float cutoff) {
  // Only update if cutoff changed significantly (to avoid constant recalculation)
  if (fabs(cutoff - lastCutoff) > 1.0) {
    calculateCoefficients(cutoff);
    lastCutoff = cutoff;
  }
}

void BiquadFilter::calculateCoefficients(float cutoff) {
  float w0 = 2.0 * PI * cutoff / SAMPLE_RATE;
  float cos_w0 = cos(w0);
  float sin_w0 = sin(w0);
  float alpha = sin_w0 / (2.0 * FILTER_Q);
  
  float b0 = (1.0 - cos_w0) / 2.0;
  float b1 = 1.0 - cos_w0;
  float b2 = (1.0 - cos_w0) / 2.0;
  float a0 = 1.0 + alpha;
  float a1 = -2.0 * cos_w0;
  float a2 = 1.0 - alpha;
  
  // Normalize coefficients and apply gain compensation
  // Gain compensation reduces the resonant peak's volume boost to prevent sudden jumps
  float norm = 1.0 / a0;
  state.a0 = b0 * norm * FILTER_GAIN_COMP;
  state.a1 = b1 * norm * FILTER_GAIN_COMP;
  state.a2 = b2 * norm * FILTER_GAIN_COMP;
  state.b1 = a1 * norm;
  state.b2 = a2 * norm;
}

float BiquadFilter::process(float input) {
  float output = state.a0 * input + 
                 state.a1 * state.x1 + 
                 state.a2 * state.x2 -
                 state.b1 * state.y1 -
                 state.b2 * state.y2;
  
  // Update history
  state.x2 = state.x1;
  state.x1 = input;
  state.y2 = state.y1;
  state.y1 = output;
  
  return output;
}

