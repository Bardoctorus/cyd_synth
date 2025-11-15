/*
 * Parameter Control UI
 * Handles parameter sliders in the bottom half of the screen
 */

#ifndef PARAMETER_CONTROL_H
#define PARAMETER_CONTROL_H

#include <TFT_eSPI.h>

enum ParameterType {
  PARAM_DELAY_TIME = 0,
  PARAM_LFO_DEPTH = 1,
  PARAM_LFO_SPEED = 2,
  PARAM_BASE_NOTE = 3,
  PARAM_UPPER_TONE = 4,
  NUM_PARAMETERS
};

class ParameterControl {
public:
  ParameterControl();
  
  // Convert touch position to parameter value (with smoothing)
  // Vertical sliders: touchY controls value, touchX determines which parameter
  ParameterType touchToParameter(int touchX, int touchY, int screenWidth, int screenHeight);
  float touchToDelayTime(int touchY, int screenHeight);
  float touchToLFODepth(int touchY, int screenHeight);
  float touchToLFOSpeed(int touchY, int screenHeight);
  int touchToBaseNote(int touchY, int screenHeight);  // Returns detent index (0-4)
  int touchToUpperTone(int touchY, int screenHeight);  // Returns detent index (0-4)
  
  // Get smoothed values (for display)
  float getSmoothedDelayTime() { return smoothedDelayTime; }
  float getSmoothedLFODepth() { return smoothedLFODepth; }
  float getSmoothedLFOSpeed() { return smoothedLFOSpeed; }
  int getBaseNoteDetent() { return currentBaseNoteDetent; }
  int getUpperToneDetent() { return currentUpperToneDetent; }
  
  // Draw parameter controls in bottom half of screen
  void drawControls(TFT_eSPI& tft);
  
  // Update parameter display (show current value)
  void updateParameterDisplay(TFT_eSPI& tft, ParameterType param, float uiValue, float actualValue);
  
  // Update smoothing (call periodically)
  void updateSmoothing(ParameterType param, float targetValue);
  
  // Check if touch is in parameter control area (bottom half)
  bool isInParameterArea(int touchY, int screenHeight);
  
private:
  float smoothedDelayTime;  // Smoothed value for UI display
  float smoothedLFODepth;    // Smoothed LFO depth for UI display
  float smoothedLFOSpeed;    // Smoothed LFO speed for UI display
  int currentBaseNoteDetent;  // Current base note detent (0-4)
  int currentUpperToneDetent;  // Current upper tone detent (0-4)
  int getParameterStripWidth(int screenWidth);
  int getParameterStripX(ParameterType param, int screenWidth);
};

#endif // PARAMETER_CONTROL_H

