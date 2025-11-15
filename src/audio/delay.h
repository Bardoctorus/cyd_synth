/*
 * Stereo Delay Effect
 * DAW send-style delay with stereo width and sample rate conversion
 */

#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>

class StereoDelay {
public:
  StereoDelay();
  void init();
  void process(float inputLeft, float inputRight, float& outputLeft, float& outputRight, float delayTimeMs, float delayVarianceMs);
  
private:
  int16_t* delayBuffer;
  int delayWriteIndex;
  
  // Fractional read positions for variable delay time (smooth transitions)
  float delayReadPosLeft;   // Fractional read position (left channel)
  float delayReadPosRight;  // Fractional read position (right channel)
  float delayReadSpeedLeft;  // Read speed adjustment (left channel, typically 0.95-1.05)
  float delayReadSpeedRight; // Read speed adjustment (right channel, typically 0.95-1.05)
  
  // Sample rate conversion state
  int delayWriteCounter;
  int delayReadCounterLeft;
  int delayReadCounterRight;
  float delayReadLastLeft;
  float delayReadLastRight;
  
  void readDelay(float& left, float& right, float delayTimeMs, float delayVarianceMs);
  void writeDelay(float input);
};

#endif // DELAY_H

