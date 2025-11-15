/*
 * Biquad Low-Pass Filter
 * Resonant low-pass filter with gain compensation
 */

#ifndef FILTER_H
#define FILTER_H

class BiquadFilter {
public:
  BiquadFilter();
  void setCutoff(float cutoff);  // Update filter cutoff frequency
  float process(float input);     // Process a single sample
  
private:
  struct BiquadState {
    float x1, x2;  // Input history
    float y1, y2;  // Output history
    float a0, a1, a2, b1, b2;  // Filter coefficients
  };
  
  BiquadState state;
  float lastCutoff;
  
  void calculateCoefficients(float cutoff);
};

#endif // FILTER_H

