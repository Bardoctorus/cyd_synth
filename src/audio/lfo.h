/*
 * Low Frequency Oscillator (LFO)
 * Generates sine wave for modulation effects
 */

#ifndef LFO_H
#define LFO_H

class LFO {
public:
  LFO();
  void setSpeed(float frequencyHz);  // Set LFO speed (0.1 to 50 Hz)
  float process();  // Generate next sample, returns -1.0 to 1.0 (sine wave)
  
private:
  float phase;      // Phase accumulator
  float phaseInc;   // Phase increment per sample (calculated from frequency)
};

#endif // LFO_H

