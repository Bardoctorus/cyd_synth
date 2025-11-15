/*
 * Dual Oscillator Voice Generator
 * Generates sine + sawtooth waves (sawtooth one octave above)
 */

#ifndef OSCILLATOR_H
#define OSCILLATOR_H

class Oscillator {
public:
  Oscillator();
  float generate(float frequency, bool active);  // Generate next sample, returns 0.0 if not active
  
private:
  float phase;      // Phase for sine wave
  float sawPhase;   // Phase for sawtooth wave (one octave above)
};

#endif // OSCILLATOR_H

