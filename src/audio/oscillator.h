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
  void setSemitoneInterval(float semitones);  // Set sawtooth interval in semitones (updates ratio)
  
private:
  float phase;      // Phase for sine wave
  float sawPhase;   // Phase for sawtooth wave
  float semitoneRatio;  // Frequency ratio for sawtooth (calculated from semitone interval, cached to avoid pow() in audio loop)
};

#endif // OSCILLATOR_H

