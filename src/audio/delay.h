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
  void process(float inputLeft, float inputRight, float& outputLeft, float& outputRight);
  
private:
  int16_t* delayBuffer;
  int delayWriteIndex;
  int delayReadIndexLeft;
  int delayReadIndexRight;
  
  // Sample rate conversion state
  int delayWriteCounter;
  int delayReadCounterLeft;
  int delayReadCounterRight;
  float delayReadLastLeft;
  float delayReadLastRight;
  
  void readDelay(float& left, float& right);
  void writeDelay(float input);
};

#endif // DELAY_H

