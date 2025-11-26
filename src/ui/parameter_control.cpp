/*
 * Parameter Control UI Implementation
 */

#include "parameter_control.h"
#include "../config.h"
#include <math.h>
#include <string.h>

ParameterControl::ParameterControl() 
  : smoothedDelayTime(DELAY_TIME_MS_DEFAULT),
    smoothedDelayFeedback(DELAY_FEEDBACK),
    smoothedLFODepth(LFO_DEPTH_MS_DEFAULT),
    smoothedLFOSpeed(LFO_SPEED_HZ_DEFAULT),
    currentBaseNoteDetent(BASE_NOTE_DEFAULT),
    currentUpperToneDetent(UPPER_TONE_DEFAULT) {
}

bool ParameterControl::isInParameterArea(int touchY, int screenHeight) {
  int controlAreaHeight = screenHeight / 2;
  return touchY >= controlAreaHeight;
}

int ParameterControl::getParameterStripWidth(int screenWidth) {
  // Use a fixed strip width for each fader region, leaving empty space on the right
  (void)screenWidth;  // Unused (kept for interface compatibility)
  return FADER_STRIP_WIDTH;
}

int ParameterControl::getParameterStripX(ParameterType param, int screenWidth) {
  int stripWidth = getParameterStripWidth(screenWidth);
  // Start immediately to the right of the menu button
  return MENU_BUTTON_WIDTH + param * stripWidth;
}

ParameterType ParameterControl::touchToParameter(int touchX, int touchY, int screenWidth, int screenHeight) {
  // Determine which parameter strip the touch is in (based on X position)
  int stripWidth = getParameterStripWidth(screenWidth);
  // Shift touch coordinate so 0 starts at the right edge of the menu button
  int localX = touchX - MENU_BUTTON_WIDTH;
  if (localX < 0) localX = 0;
  int stripIndex = localX / stripWidth;
  
  // Clamp to valid parameter range
  if (stripIndex < 0) stripIndex = 0;
  if (stripIndex >= NUM_PARAMETERS) stripIndex = NUM_PARAMETERS - 1;
  
  return (ParameterType)stripIndex;
}

float ParameterControl::touchToDelayTime(int touchY, int screenHeight) {
  // Map touch Y position (in bottom half) to delay time (50ms to 1000ms)
  // Vertical slider: top of bottom half = max delay, bottom = min delay
  int controlAreaHeight = screenHeight / 2;
  int controlAreaY = controlAreaHeight;
  int relativeY = touchY - controlAreaY;
  float yPos = 1.0 - ((float)relativeY / (float)controlAreaHeight);  // Invert: top = 1.0, bottom = 0.0
  yPos = constrain(yPos, 0.0, 1.0);
  
  // Logarithmic scaling: convert linear 0-1 to logarithmic delay time range
  float logMin = log10(DELAY_TIME_MS_MIN);
  float logMax = log10(DELAY_TIME_MS_MAX);
  float logRange = logMax - logMin;
  
  // Map linear position to logarithmic delay time
  float logDelay = logMin + yPos * logRange;
  float delayTime = pow(10.0, logDelay);
  
  delayTime = constrain(delayTime, DELAY_TIME_MS_MIN, DELAY_TIME_MS_MAX);
  
  // Update smoothed value for UI display
  updateSmoothing(PARAM_DELAY_TIME, delayTime);
  
  return delayTime;
}

float ParameterControl::touchToLFODepth(int touchY, int screenHeight) {
  // Map touch Y position (in bottom half) to LFO depth (0-100ms)
  // Vertical slider: top of bottom half = max depth, bottom = min depth
  int controlAreaHeight = screenHeight / 2;
  int controlAreaY = controlAreaHeight;
  int relativeY = touchY - controlAreaY;
  float yPos = 1.0 - ((float)relativeY / (float)controlAreaHeight);  // Invert: top = 1.0, bottom = 0.0
  yPos = constrain(yPos, 0.0, 1.0);
  
  // Linear mapping: 0.0 to 1.0 -> LFO_DEPTH_MS_MIN to LFO_DEPTH_MS_MAX
  float lfoDepth = LFO_DEPTH_MS_MIN + yPos * (LFO_DEPTH_MS_MAX - LFO_DEPTH_MS_MIN);
  lfoDepth = constrain(lfoDepth, LFO_DEPTH_MS_MIN, LFO_DEPTH_MS_MAX);
  
  // Update smoothed value for UI display
  updateSmoothing(PARAM_LFO_DEPTH, lfoDepth);
  
  return lfoDepth;
}

float ParameterControl::touchToDelayFeedback(int touchY, int screenHeight) {
  // Map touch Y position (in bottom half) to delay feedback (1% - 100%)
  // Vertical slider: top of bottom half = max feedback, bottom = min feedback
  int controlAreaHeight = screenHeight / 2;
  int controlAreaY = controlAreaHeight;
  int relativeY = touchY - controlAreaY;
  float yPos = 1.0 - ((float)relativeY / (float)controlAreaHeight);  // Invert: top = 1.0, bottom = 0.0
  yPos = constrain(yPos, 0.0, 1.0);
  
  // Linear mapping: 0.0 to 1.0 -> DELAY_FEEDBACK_MIN to DELAY_FEEDBACK_MAX
  float feedback = DELAY_FEEDBACK_MIN + yPos * (DELAY_FEEDBACK_MAX - DELAY_FEEDBACK_MIN);
  feedback = constrain(feedback, DELAY_FEEDBACK_MIN, DELAY_FEEDBACK_MAX);
  
  // Update smoothed value for UI display
  updateSmoothing(PARAM_DELAY_FEEDBACK, feedback);
  
  return feedback;
}

float ParameterControl::touchToLFOSpeed(int touchY, int screenHeight) {
  // Map touch Y position (in bottom half) to LFO speed (0.1-50 Hz)
  // Vertical slider: top of bottom half = max speed, bottom = min speed
  int controlAreaHeight = screenHeight / 2;
  int controlAreaY = controlAreaHeight;
  int relativeY = touchY - controlAreaY;
  float yPos = 1.0 - ((float)relativeY / (float)controlAreaHeight);  // Invert: top = 1.0, bottom = 0.0
  yPos = constrain(yPos, 0.0, 1.0);
  
  // Logarithmic scaling for frequency (more natural feel)
  float logMin = log10(LFO_SPEED_HZ_MIN);
  float logMax = log10(LFO_SPEED_HZ_MAX);
  float logRange = logMax - logMin;
  
  // Map linear position to logarithmic LFO speed
  float logSpeed = logMin + yPos * logRange;
  float lfoSpeed = pow(10.0, logSpeed);
  
  lfoSpeed = constrain(lfoSpeed, LFO_SPEED_HZ_MIN, LFO_SPEED_HZ_MAX);
  
  // Update smoothed value for UI display
  updateSmoothing(PARAM_LFO_SPEED, lfoSpeed);
  
  return lfoSpeed;
}

int ParameterControl::touchToBaseNote(int touchY, int screenHeight) {
  // Map touch Y position to one of 5 detents
  // Vertical slider: top = detent 4, bottom = detent 0
  int controlAreaHeight = screenHeight / 2;
  int controlAreaY = controlAreaHeight;
  int relativeY = touchY - controlAreaY;
  float yPos = 1.0 - ((float)relativeY / (float)controlAreaHeight);  // Invert: top = 1.0, bottom = 0.0
  yPos = constrain(yPos, 0.0, 1.0);
  
  // Map to 5 detents (0-4)
  int detent = (int)(yPos * BASE_NOTE_DETENTS);
  if (detent >= BASE_NOTE_DETENTS) detent = BASE_NOTE_DETENTS - 1;
  
  currentBaseNoteDetent = detent;
  return detent;
}

int ParameterControl::touchToUpperTone(int touchY, int screenHeight) {
  // Map touch Y position to one of 5 detents
  // Vertical slider: top = detent 4, bottom = detent 0
  int controlAreaHeight = screenHeight / 2;
  int controlAreaY = controlAreaHeight;
  int relativeY = touchY - controlAreaY;
  float yPos = 1.0 - ((float)relativeY / (float)controlAreaHeight);  // Invert: top = 1.0, bottom = 0.0
  yPos = constrain(yPos, 0.0, 1.0);
  
  // Map to 5 detents (0-4)
  int detent = (int)(yPos * UPPER_TONE_DETENTS);
  if (detent >= UPPER_TONE_DETENTS) detent = UPPER_TONE_DETENTS - 1;
  
  currentUpperToneDetent = detent;
  return detent;
}

void ParameterControl::updateSmoothing(ParameterType param, float targetValue) {
  // Smooth the value for UI display (makes slider move smoothly even with rapid touch changes)
  // Smoothing rate: 0.1 means 10% per update (50ms intervals = smooth but responsive)
  float smoothingRate = 0.1;
  
  switch (param) {
    case PARAM_DELAY_TIME:
      smoothedDelayTime += (targetValue - smoothedDelayTime) * smoothingRate;
      break;
    case PARAM_DELAY_FEEDBACK:
      smoothedDelayFeedback += (targetValue - smoothedDelayFeedback) * smoothingRate;
      break;
    case PARAM_LFO_DEPTH:
      smoothedLFODepth += (targetValue - smoothedLFODepth) * smoothingRate;
      break;
    case PARAM_LFO_SPEED:
      smoothedLFOSpeed += (targetValue - smoothedLFOSpeed) * smoothingRate;
      break;
    default:
      break;
  }
}

// Helper to draw a vertical label string (one character per row)
static void drawVerticalLabel(TFT_eSPI& tft, const char* text, int x, int yStart) {
  int len = strlen(text);
  int charHeight = 8;  // Approximate height for default font at textSize=1
  for (int i = 0; i < len; i++) {
    char c = text[i];
    tft.setCursor(x, yStart + i * charHeight);
    tft.print(c);
  }
}

void ParameterControl::drawControls(TFT_eSPI& tft) {
  int screenHeight = tft.height();
  int screenWidth = tft.width();
  int controlAreaHeight = screenHeight / 2;
  int controlAreaY = controlAreaHeight;
  int stripWidth = getParameterStripWidth(screenWidth);
  const int labelColumnWidth = 10;  // Pixels reserved at left of each strip for vertical text
  
  // Draw divider line between top and bottom halves
  tft.drawLine(0, controlAreaY, screenWidth, controlAreaY, TFT_WHITE);

  // Draw vertical divider at right edge of menu button (for clarity)
  int menuDividerX = MENU_BUTTON_WIDTH;
  tft.drawLine(menuDividerX, controlAreaY, menuDividerX, screenHeight - 1, TFT_WHITE);

  // Draw vertical dividers between parameter strips (to the right of menu)
  for (int i = 1; i < NUM_PARAMETERS; i++) {
    int x = MENU_BUTTON_WIDTH + i * stripWidth;
    tft.drawLine(x, controlAreaY, x, screenHeight - 1, TFT_DARKGREY);
  }
  
  // Draw labels for each parameter
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  
  // Draw vertical labels for each parameter
  // Labels are drawn near the left side of each fader area, against the divider
  int labelTopY = controlAreaY + 5;

  // Delay Time label
  int delayX = getParameterStripX(PARAM_DELAY_TIME, screenWidth);
  drawVerticalLabel(tft, "Delay", delayX + 2, labelTopY);

  // Delay Feedback label
  int feedbackX = getParameterStripX(PARAM_DELAY_FEEDBACK, screenWidth);
  drawVerticalLabel(tft, "Feedback", feedbackX + 2, labelTopY);
  
  // LFO Depth label
  int depthX = getParameterStripX(PARAM_LFO_DEPTH, screenWidth);
  drawVerticalLabel(tft, "LFO Depth", depthX + 2, labelTopY);
  
  // LFO Speed label
  int speedX = getParameterStripX(PARAM_LFO_SPEED, screenWidth);
  drawVerticalLabel(tft, "LFO Speed", speedX + 2, labelTopY);
  
  // Base Tone label
  int baseNoteX = getParameterStripX(PARAM_BASE_NOTE, screenWidth);
  drawVerticalLabel(tft, "Base Tone", baseNoteX + 2, labelTopY);
  
  // Upper Tone label
  int upperToneX = getParameterStripX(PARAM_UPPER_TONE, screenWidth);
  drawVerticalLabel(tft, "Upper Tone", upperToneX + 2, labelTopY);
  
  // Draw initial sliders and values (UI and actual values are same at startup)
  updateParameterDisplay(tft, PARAM_DELAY_TIME, DELAY_TIME_MS_DEFAULT, DELAY_TIME_MS_DEFAULT);
  updateParameterDisplay(tft, PARAM_DELAY_FEEDBACK, DELAY_FEEDBACK, DELAY_FEEDBACK);
  updateParameterDisplay(tft, PARAM_LFO_DEPTH, LFO_DEPTH_MS_DEFAULT, LFO_DEPTH_MS_DEFAULT);
  updateParameterDisplay(tft, PARAM_LFO_SPEED, LFO_SPEED_HZ_DEFAULT, LFO_SPEED_HZ_DEFAULT);
  updateParameterDisplay(tft, PARAM_BASE_NOTE, (float)BASE_NOTE_DEFAULT, (float)BASE_NOTE_DEFAULT);
  updateParameterDisplay(tft, PARAM_UPPER_TONE, (float)UPPER_TONE_DEFAULT, (float)UPPER_TONE_DEFAULT);
}

void ParameterControl::updateParameterDisplay(TFT_eSPI& tft, ParameterType param, float uiValue, float actualValue) {
  int screenHeight = tft.height();
  int screenWidth = tft.width();
  int controlAreaHeight = screenHeight / 2;
  int controlAreaY = controlAreaHeight;
  int stripWidth = getParameterStripWidth(screenWidth);
  int stripX = getParameterStripX(param, screenWidth);
  const int labelColumnWidth = 10;  // Match drawControls
  
  // Clear old value and slider area for this parameter
  // Leave the left label column untouched so vertical text is not erased
  int clearX = stripX + labelColumnWidth + 2;
  int clearW = stripWidth - (labelColumnWidth + 4);
  if (clearW < 0) clearW = 0;
  tft.fillRect(clearX, controlAreaY + 15, clearW, controlAreaHeight - 20, TFT_BLACK);
  
  // Draw slider track (vertical)
  // Place track inside the fader region, with a 2-pixel gap from label area on the left
  // and a 2-pixel gap from the right divider so the thumb never touches the line.
  // Pattern per strip: divider | 2px gap | vertical text | 2px gap | fader | 2px gap | next divider
  int faderLeft = stripX + labelColumnWidth + 3;   // 2px gap after label column
  int faderRight = stripX + stripWidth - 3;        // 2px gap before next divider
  if (faderRight < faderLeft) faderRight = faderLeft;
  int trackX = (faderLeft + faderRight) / 2;
  tft.drawLine(trackX, controlAreaY + 20, trackX, screenHeight - 10, TFT_DARKGREY);
  
  // Draw detent dots for base tone and upper tone (5 detents)
  if (param == PARAM_BASE_NOTE || param == PARAM_UPPER_TONE) {
    int numDetents = (param == PARAM_BASE_NOTE) ? BASE_NOTE_DETENTS : UPPER_TONE_DETENTS;
    int trackStartY = controlAreaY + 20;
    int trackEndY = screenHeight - 10;
    int trackHeight = trackEndY - trackStartY;
    int detentSpacing = trackHeight / (numDetents - 1);
    
    for (int i = 0; i < numDetents; i++) {
      int dotY = trackStartY + (i * detentSpacing);
      tft.fillCircle(trackX, dotY, 2, TFT_DARKGREY);
    }
  }
  
  // Calculate slider position based on parameter type (using UI value)
  float normalizedPos = 0.0;
  char uiValueStr[20];
  char actualValueStr[20];
  
  if (param == PARAM_DELAY_TIME) {
    // Logarithmic mapping for delay time
    float logMin = log10(DELAY_TIME_MS_MIN);
    float logMax = log10(DELAY_TIME_MS_MAX);
    float logValue = log10(uiValue);
    normalizedPos = (logValue - logMin) / (logMax - logMin);
    snprintf(uiValueStr, sizeof(uiValueStr), "%.0f ms", uiValue);
    snprintf(actualValueStr, sizeof(actualValueStr), "%.0f ms", actualValue);
  } else if (param == PARAM_DELAY_FEEDBACK) {
    // Linear mapping for delay feedback
    normalizedPos = (uiValue - DELAY_FEEDBACK_MIN) / (DELAY_FEEDBACK_MAX - DELAY_FEEDBACK_MIN);
  } else if (param == PARAM_LFO_DEPTH) {
    // Linear mapping for LFO depth
    normalizedPos = (uiValue - LFO_DEPTH_MS_MIN) / (LFO_DEPTH_MS_MAX - LFO_DEPTH_MS_MIN);
    snprintf(uiValueStr, sizeof(uiValueStr), "%.0f ms", uiValue);
    snprintf(actualValueStr, sizeof(actualValueStr), "%.0f ms", actualValue);
  } else if (param == PARAM_LFO_SPEED) {
    // Logarithmic mapping for LFO speed
    float logMin = log10(LFO_SPEED_HZ_MIN);
    float logMax = log10(LFO_SPEED_HZ_MAX);
    float logValue = log10(uiValue);
    normalizedPos = (logValue - logMin) / (logMax - logMin);
    snprintf(uiValueStr, sizeof(uiValueStr), "%.1f Hz", uiValue);
    snprintf(actualValueStr, sizeof(actualValueStr), "%.1f Hz", actualValue);
  } else if (param == PARAM_BASE_NOTE) {
    // Base tone: 5 detents, map detent index to position
    int detent = (int)uiValue;
    normalizedPos = (float)detent / (float)(BASE_NOTE_DETENTS - 1);
    
    // Calculate frequency for this detent
    // Middle (detent 2) = MIN_FREQ_BASE
    // Each step is a whole tone (2 semitones)
    int stepsFromMiddle = detent - BASE_NOTE_DEFAULT;
    float freq = MIN_FREQ_BASE * pow(WHOLE_TONE_RATIO, stepsFromMiddle);
    
    // Show frequency value
    snprintf(uiValueStr, sizeof(uiValueStr), "%.0fHz", freq);
    snprintf(actualValueStr, sizeof(actualValueStr), "%.0fHz", freq);
  } else if (param == PARAM_UPPER_TONE) {
    // Upper tone: 5 detents, map detent index to position
    int detent = (int)uiValue;
    normalizedPos = (float)detent / (float)(UPPER_TONE_DETENTS - 1);
    
    float semitones = UPPER_TONE_SEMITONES[detent];
    snprintf(uiValueStr, sizeof(uiValueStr), "%.0fst", semitones);
    snprintf(actualValueStr, sizeof(actualValueStr), "%.0fst", semitones);
  }
  
  // Invert normalized position (top = 1.0, bottom = 0.0)
  normalizedPos = 1.0 - normalizedPos;
  normalizedPos = constrain(normalizedPos, 0.0, 1.0);
  
  // Calculate slider thumb Y position
  int sliderY = controlAreaY + 20 + (int)(normalizedPos * (controlAreaHeight - 30));
  
  // Draw slider thumb
  tft.fillCircle(trackX, sliderY, 5, TFT_CYAN);
  tft.drawCircle(trackX, sliderY, 5, TFT_WHITE);
  
  // Fader value text is intentionally disabled for now to keep the layout compact
}
