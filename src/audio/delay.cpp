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
                             delayReadPosLeft(0.0), delayReadPosRight(0.0),
                             delayReadSpeedLeft(1.0), delayReadSpeedRight(1.0),
                             delayWriteCounter(0), delayReadCounterLeft(0), 
                             delayReadCounterRight(0), delayReadLastLeft(0.0), 
                             delayReadLastRight(0.0) {
}

void StereoDelay::init() {
  // Allocate delay buffer (maximum size to allow real-time delay changes)
  delayBuffer = (int16_t*)malloc(DELAY_BUFFER_SIZE * sizeof(int16_t));
  if (!delayBuffer) {
    Serial2.printf("ERROR: Failed to allocate delay buffer (%d samples, %.2f KB)\n", 
                   DELAY_BUFFER_SIZE, (DELAY_BUFFER_SIZE * sizeof(int16_t)) / 1024.0);
    return;
  }
  memset(delayBuffer, 0, DELAY_BUFFER_SIZE * sizeof(int16_t));
  
  // Initialize fractional read positions based on default delay time
  // Start read positions behind write position by the delay amount
  float defaultDelaySamples = DELAY_TIME_MS_DEFAULT * DELAY_SAMPLE_RATE / 1000.0;
  float defaultVarianceSamples = DELAY_TIME_VARIANCE_MS * DELAY_SAMPLE_RATE / 1000.0;
  float delayLeftSamples = defaultDelaySamples - defaultVarianceSamples;
  float delayRightSamples = defaultDelaySamples + defaultVarianceSamples;
  
  // Set initial read positions (behind write position in circular buffer)
  int delayLeftSamplesInt = (int)delayLeftSamples;
  int delayRightSamplesInt = (int)delayRightSamples;
  delayReadPosLeft = (float)((DELAY_BUFFER_SIZE - delayLeftSamplesInt) % DELAY_BUFFER_SIZE);
  delayReadPosRight = (float)((DELAY_BUFFER_SIZE - delayRightSamplesInt) % DELAY_BUFFER_SIZE);
  delayReadSpeedLeft = 1.0;
  delayReadSpeedRight = 1.0;
  
  // Initialize sample rate conversion counters
  delayWriteCounter = 0;
  delayReadCounterLeft = 0;
  delayReadCounterRight = 0;
  delayReadLastLeft = 0.0;
  delayReadLastRight = 0.0;
}

void StereoDelay::readDelay(float& left, float& right, float delayTimeMs, float delayVarianceMs) {
  if (!delayBuffer) {
    left = 0.0;
    right = 0.0;
    return;
  }
  
  if (DELAY_SAMPLE_RATE == SAMPLE_RATE) {
    // Same sample rate: use variable read speed directly
    // Calculate target delay in samples
    float targetDelaySamples = delayTimeMs * DELAY_SAMPLE_RATE / 1000.0;
    float varianceSamples = delayVarianceMs * DELAY_SAMPLE_RATE / 1000.0;
    float targetDelayLeft = targetDelaySamples - varianceSamples;
    float targetDelayRight = targetDelaySamples + varianceSamples;
    
    // Calculate current delay (how many samples behind write position in circular buffer)
    float currentDelayLeft = delayReadPosLeft - (float)delayWriteIndex;
    if (currentDelayLeft < 0) currentDelayLeft += DELAY_BUFFER_SIZE;
    if (currentDelayLeft >= DELAY_BUFFER_SIZE) currentDelayLeft -= DELAY_BUFFER_SIZE;
    
    float currentDelayRight = delayReadPosRight - (float)delayWriteIndex;
    if (currentDelayRight < 0) currentDelayRight += DELAY_BUFFER_SIZE;
    if (currentDelayRight >= DELAY_BUFFER_SIZE) currentDelayRight -= DELAY_BUFFER_SIZE;
    
    // Calculate delay error and adjust read speed smoothly
    float delayErrorLeft = targetDelayLeft - currentDelayLeft;
    float delayErrorRight = targetDelayRight - currentDelayRight;
    
    // Calculate target read speed (how fast we need to read to reach target delay)
    // Reduced adjustment rate for smoother transitions (0.005 = very gradual)
    float adjustmentRate = 0.005;  // Reduced from 0.02 for much smoother transitions
    float targetSpeedLeft = 1.0 + (delayErrorLeft * adjustmentRate);
    float targetSpeedRight = 1.0 + (delayErrorRight * adjustmentRate);
    
    // Limit speed to reasonable range (expanded slightly for smoother transitions)
    targetSpeedLeft = constrain(targetSpeedLeft, 0.3, 3.0);  // Expanded from 0.5-2.0
    targetSpeedRight = constrain(targetSpeedRight, 0.3, 3.0);
    
    // Smoothly transition read speed (slower transition for smoother sound)
    float transitionRate = 0.01;  // Reduced from 0.05 for much smoother transitions
    delayReadSpeedLeft += (targetSpeedLeft - delayReadSpeedLeft) * transitionRate;
    delayReadSpeedRight += (targetSpeedRight - delayReadSpeedRight) * transitionRate;
    
    // Read with linear interpolation
    int index0Left = (int)delayReadPosLeft;
    int index1Left = (index0Left + 1) % DELAY_BUFFER_SIZE;
    float fracLeft = delayReadPosLeft - index0Left;
    float sample0Left = (float)delayBuffer[index0Left] / 32768.0;
    float sample1Left = (float)delayBuffer[index1Left] / 32768.0;
    left = sample0Left * (1.0 - fracLeft) + sample1Left * fracLeft;
    
    int index0Right = (int)delayReadPosRight;
    int index1Right = (index0Right + 1) % DELAY_BUFFER_SIZE;
    float fracRight = delayReadPosRight - index0Right;
    float sample0Right = (float)delayBuffer[index0Right] / 32768.0;
    float sample1Right = (float)delayBuffer[index1Right] / 32768.0;
    right = sample0Right * (1.0 - fracRight) + sample1Right * fracRight;
    
    // Advance read positions at variable speed
    delayReadPosLeft += delayReadSpeedLeft;
    delayReadPosRight += delayReadSpeedRight;
    
    // Handle buffer wraparound
    if (delayReadPosLeft >= DELAY_BUFFER_SIZE) delayReadPosLeft -= DELAY_BUFFER_SIZE;
    if (delayReadPosLeft < 0) delayReadPosLeft += DELAY_BUFFER_SIZE;
    if (delayReadPosRight >= DELAY_BUFFER_SIZE) delayReadPosRight -= DELAY_BUFFER_SIZE;
    if (delayReadPosRight < 0) delayReadPosRight += DELAY_BUFFER_SIZE;
  } else {
    // Different sample rates: need upsampling (delay is lower rate)
    // Read position and speed are in delay sample rate space
    float sampleRateRatio = (float)SAMPLE_RATE / DELAY_SAMPLE_RATE;
    
    // Calculate target delay in samples (in delay sample rate space)
    float targetDelaySamples = delayTimeMs * DELAY_SAMPLE_RATE / 1000.0;
    float varianceSamples = delayVarianceMs * DELAY_SAMPLE_RATE / 1000.0;
    float targetDelayLeft = targetDelaySamples - varianceSamples;
    float targetDelayRight = targetDelaySamples + varianceSamples;
    
    // Calculate current delay (in delay sample rate space)
    // delayWriteIndex advances at delay sample rate, so this is correct
    float currentDelayLeft = delayReadPosLeft - (float)delayWriteIndex;
    if (currentDelayLeft < 0) currentDelayLeft += DELAY_BUFFER_SIZE;
    if (currentDelayLeft >= DELAY_BUFFER_SIZE) currentDelayLeft -= DELAY_BUFFER_SIZE;
    
    float currentDelayRight = delayReadPosRight - (float)delayWriteIndex;
    if (currentDelayRight < 0) currentDelayRight += DELAY_BUFFER_SIZE;
    if (currentDelayRight >= DELAY_BUFFER_SIZE) currentDelayRight -= DELAY_BUFFER_SIZE;
    
    // Calculate delay error and adjust read speed smoothly
    float delayErrorLeft = targetDelayLeft - currentDelayLeft;
    float delayErrorRight = targetDelayRight - currentDelayRight;
    
    // Calculate target read speed (in delay sample rate space)
    // Reduced adjustment rate for smoother transitions
    float adjustmentRate = 0.005;  // Reduced from 0.02 for much smoother transitions
    float targetSpeedLeft = 1.0 + (delayErrorLeft * adjustmentRate);
    float targetSpeedRight = 1.0 + (delayErrorRight * adjustmentRate);
    
    // Limit speed to reasonable range (expanded for smoother transitions)
    targetSpeedLeft = constrain(targetSpeedLeft, 0.3, 3.0);  // Expanded from 0.5-2.0
    targetSpeedRight = constrain(targetSpeedRight, 0.3, 3.0);
    
    // Smoothly transition read speed (slower transition for smoother sound)
    float transitionRate = 0.01;  // Reduced from 0.05 for much smoother transitions
    delayReadSpeedLeft += (targetSpeedLeft - delayReadSpeedLeft) * transitionRate;
    delayReadSpeedRight += (targetSpeedRight - delayReadSpeedRight) * transitionRate;
    
    // Update read positions only when counter reaches sampleRateRatio (delay sample rate)
    // Left channel
    if (delayReadCounterLeft >= (int)sampleRateRatio) {
      delayReadCounterLeft = 0;
      
      // Read with linear interpolation (in delay sample rate space)
      int index0Left = (int)delayReadPosLeft;
      int index1Left = (index0Left + 1) % DELAY_BUFFER_SIZE;
      float fracLeft = delayReadPosLeft - index0Left;
      float sample0Left = (float)delayBuffer[index0Left] / 32768.0;
      float sample1Left = (float)delayBuffer[index1Left] / 32768.0;
      delayReadLastLeft = sample0Left * (1.0 - fracLeft) + sample1Left * fracLeft;
      
      // Advance read position at variable speed (in delay sample rate space)
      delayReadPosLeft += delayReadSpeedLeft;
      if (delayReadPosLeft >= DELAY_BUFFER_SIZE) delayReadPosLeft -= DELAY_BUFFER_SIZE;
      if (delayReadPosLeft < 0) delayReadPosLeft += DELAY_BUFFER_SIZE;
    }
    left = delayReadLastLeft;  // Hold last value for upsampling
    delayReadCounterLeft++;
    
    // Right channel
    if (delayReadCounterRight >= (int)sampleRateRatio) {
      delayReadCounterRight = 0;
      
      // Read with linear interpolation (in delay sample rate space)
      int index0Right = (int)delayReadPosRight;
      int index1Right = (index0Right + 1) % DELAY_BUFFER_SIZE;
      float fracRight = delayReadPosRight - index0Right;
      float sample0Right = (float)delayBuffer[index0Right] / 32768.0;
      float sample1Right = (float)delayBuffer[index1Right] / 32768.0;
      delayReadLastRight = sample0Right * (1.0 - fracRight) + sample1Right * fracRight;
      
      // Advance read position at variable speed (in delay sample rate space)
      delayReadPosRight += delayReadSpeedRight;
      if (delayReadPosRight >= DELAY_BUFFER_SIZE) delayReadPosRight -= DELAY_BUFFER_SIZE;
      if (delayReadPosRight < 0) delayReadPosRight += DELAY_BUFFER_SIZE;
    }
    right = delayReadLastRight;  // Hold last value for upsampling
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

void StereoDelay::process(float inputLeft, float inputRight, float& outputLeft, float& outputRight, float delayTimeMs, float delayVarianceMs) {
  // Read delayed signals from buffer (with stereo width and variable delay time) - delay return (no filter)
  readDelay(outputLeft, outputRight, delayTimeMs, delayVarianceMs);
  
  // Mix filtered instrument with feedback from delay (feedback continues even when not touching)
  // Use average of left and right for mono feedback
  float delayFeedback = (outputLeft + outputRight) / 2.0;
  float delayInput = (inputLeft * DELAY_SEND_LEVEL) + (delayFeedback * DELAY_FEEDBACK);
  
  // Write to delay buffer
  writeDelay(delayInput);
}

