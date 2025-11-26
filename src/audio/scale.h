/*
 * Scale Abstraction
 * Abstract interface for different musical scales
 * This allows easy switching between pentatonic, chromatic, major, etc.
 */

#ifndef SCALE_H
#define SCALE_H

#include "../config/settings.h"

class Scale {
public:
  virtual ~Scale() {}
  
  // Quantize a frequency to the nearest note in the scale
  virtual float quantize(float frequency) = 0;
  
  // Get the number of notes per octave in this scale
  virtual int getNotesPerOctave() = 0;
  
  // Get the frequency ratios for this scale (normalized to root = 1.0)
  // Returns array of ratios and number of ratios via output parameters
  virtual void getRatios(float* ratios, int& numRatios) = 0;
  
  // Get scale type
  virtual ScaleType getType() = 0;
  
  // Set base frequency (lowest note in scale)
  virtual void setBaseFrequency(float baseFreq) = 0;
  
  // Get base frequency
  virtual float getBaseFrequency() = 0;
};

// Pentatonic scale implementation (current default)
class PentatonicScale : public Scale {
public:
  PentatonicScale();
  float quantize(float frequency) override;
  int getNotesPerOctave() override { return 5; }
  void getRatios(float* ratios, int& numRatios) override;
  ScaleType getType() override { return SCALE_PENTATONIC; }
  void setBaseFrequency(float baseFreq) override;
  float getBaseFrequency() override { return currentBaseFreq; }
  
private:
  float currentBaseFreq;
  static const int NUM_RATIOS = 5;
  static const float RATIOS[5];  // Pentatonic ratios
};

// Chromatic scale implementation (12 notes per octave)
class ChromaticScale : public Scale {
public:
  ChromaticScale();
  float quantize(float frequency) override;
  int getNotesPerOctave() override { return 12; }
  void getRatios(float* ratios, int& numRatios) override;
  ScaleType getType() override { return SCALE_CHROMATIC; }
  void setBaseFrequency(float baseFreq) override;
  float getBaseFrequency() override { return currentBaseFreq; }
  
private:
  float currentBaseFreq;
  static const int NUM_RATIOS = 12;
  static const float RATIOS[12];  // Chromatic ratios (12 semitones)
};

// Factory function to create scale based on type
Scale* createScale(ScaleType type);

#endif // SCALE_H

