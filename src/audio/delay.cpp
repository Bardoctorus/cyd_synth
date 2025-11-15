/*
 * Stereo Delay Implementation
 */

#include "delay.h"
#include "../config.h"
#include <Arduino.h>
#include <HardwareSerial.h>
#include <stdlib.h>
#include <string.h>

StereoDelay::StereoDelay() : delayBuffer(nullptr), delayWriteIndex(0), 
                             delayReadIndexLeft(0), delayReadIndexRight(0),
                             delayWriteCounter(0), delayReadCounterLeft(0), 
                             delayReadCounterRight(0), delayReadLastLeft(0.0), 
                             delayReadLastRight(0.0) {
}

void StereoDelay::init() {
  // Allocate delay buffer
  delayBuffer = (int16_t*)malloc(DELAY_BUFFER_SIZE * sizeof(int16_t));
  if (!delayBuffer) {
    Serial2.printf("ERROR: Failed to allocate delay buffer (%d samples, %.2f KB)\n", 
                   DELAY_BUFFER_SIZE, (DELAY_BUFFER_SIZE * sizeof(int16_t)) / 1024.0);
    return;
  }
  memset(delayBuffer, 0, DELAY_BUFFER_SIZE * sizeof(int16_t));
  
  // Calculate delay buffer read positions for stereo width
  // Note: These are calculated in delay sample rate, not audio sample rate
  // Left channel: delay time - variance (slightly shorter delay)
  // Right channel: delay time + variance (slightly longer delay)
  int delaySamples = (int)(DELAY_TIME_MS * DELAY_SAMPLE_RATE / 1000.0);
  int varianceSamples = (int)(DELAY_TIME_VARIANCE_MS * DELAY_SAMPLE_RATE / 1000.0);
  int delayLeftSamples = delaySamples - varianceSamples;   // Left: shorter delay
  int delayRightSamples = delaySamples + varianceSamples;  // Right: longer delay
  
  // Initialize read indices (will be updated as write index advances)
  delayReadIndexLeft = (DELAY_BUFFER_SIZE - delayLeftSamples) % DELAY_BUFFER_SIZE;
  delayReadIndexRight = (DELAY_BUFFER_SIZE - delayRightSamples) % DELAY_BUFFER_SIZE;
  
  // Initialize sample rate conversion counters
  delayWriteCounter = 0;
  delayReadCounterLeft = 0;
  delayReadCounterRight = 0;
  delayReadLastLeft = 0.0;
  delayReadLastRight = 0.0;
}

void StereoDelay::readDelay(float& left, float& right) {
  if (!delayBuffer) {
    left = 0.0;
    right = 0.0;
    return;
  }
  
  if (DELAY_SAMPLE_RATE == SAMPLE_RATE) {
    // Same sample rate: direct read (convert int16 to float)
    left = (float)delayBuffer[delayReadIndexLeft] / 32768.0;
    right = (float)delayBuffer[delayReadIndexRight] / 32768.0;
    
    // Advance read indices (circular buffer)
    delayReadIndexLeft = (delayReadIndexLeft + 1) % DELAY_BUFFER_SIZE;
    delayReadIndexRight = (delayReadIndexRight + 1) % DELAY_BUFFER_SIZE;
  } else {
    // Different sample rates: need upsampling (delay is lower rate)
    float sampleRateRatio = (float)SAMPLE_RATE / DELAY_SAMPLE_RATE;
    
    // Left channel upsampling
    if (delayReadCounterLeft >= (int)sampleRateRatio) {
      delayReadCounterLeft = 0;
      delayReadLastLeft = (float)delayBuffer[delayReadIndexLeft] / 32768.0;
      delayReadIndexLeft = (delayReadIndexLeft + 1) % DELAY_BUFFER_SIZE;
    }
    left = delayReadLastLeft;  // Simple hold (can use linear interpolation for better quality)
    delayReadCounterLeft++;
    
    // Right channel upsampling
    if (delayReadCounterRight >= (int)sampleRateRatio) {
      delayReadCounterRight = 0;
      delayReadLastRight = (float)delayBuffer[delayReadIndexRight] / 32768.0;
      delayReadIndexRight = (delayReadIndexRight + 1) % DELAY_BUFFER_SIZE;
    }
    right = delayReadLastRight;  // Simple hold
    delayReadCounterRight++;
  }
}

void StereoDelay::writeDelay(float input) {
  if (!delayBuffer) return;
  
  if (DELAY_SAMPLE_RATE == SAMPLE_RATE) {
    // Same sample rate: write every sample
    // Convert float to int16_t and store
    int16_t delaySample = (int16_t)(constrain(input * 32768.0, -32768.0, 32767.0));
    delayBuffer[delayWriteIndex] = delaySample;
    delayWriteIndex = (delayWriteIndex + 1) % DELAY_BUFFER_SIZE;
  } else {
    // Different sample rates: downsample (write only every Nth sample)
    float sampleRateRatio = (float)SAMPLE_RATE / DELAY_SAMPLE_RATE;
    delayWriteCounter++;
    
    if (delayWriteCounter >= (int)sampleRateRatio) {
      delayWriteCounter = 0;
      // Convert float to int16_t and store
      int16_t delaySample = (int16_t)(constrain(input * 32768.0, -32768.0, 32767.0));
      delayBuffer[delayWriteIndex] = delaySample;
      delayWriteIndex = (delayWriteIndex + 1) % DELAY_BUFFER_SIZE;
    }
  }
}

void StereoDelay::process(float inputLeft, float inputRight, float& outputLeft, float& outputRight) {
  // Read delayed signals from buffer (with stereo width) - delay return (no filter)
  readDelay(outputLeft, outputRight);
  
  // Mix filtered instrument with feedback from delay (feedback continues even when not touching)
  // Use average of left and right for mono feedback
  float delayFeedback = (outputLeft + outputRight) / 2.0;
  float delayInput = (inputLeft * DELAY_SEND_LEVEL) + (delayFeedback * DELAY_FEEDBACK);
  
  // Write to delay buffer
  writeDelay(delayInput);
}

