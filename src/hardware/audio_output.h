/*
 * Audio Output Hardware Abstraction
 * Handles I2S/DAC initialization and audio output with safety limiting
 */

#ifndef AUDIO_OUTPUT_H
#define AUDIO_OUTPUT_H

#include <driver/i2s.h>

class AudioOutput {
public:
  AudioOutput();
  bool init();  // Returns true if successful
  void write(float leftSample, float rightSample);  // Write single stereo sample
  void writeBuffer(int16_t* buffer, size_t samples);  // Write buffer of interleaved samples
  
private:
  i2s_port_t i2sNum;
  void applyLimiter(float& left, float& right);  // Hard limit to protect DAC
};

#endif // AUDIO_OUTPUT_H

