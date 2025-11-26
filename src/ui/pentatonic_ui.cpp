/*
 * Pentatonic UI Implementation
 */

#include "pentatonic_ui.h"
#include "../config.h"
#include <math.h>

// Forward declaration for visual key count helper
static int getTotalVisualKeys();

PentatonicUI::PentatonicUI() : lastXStart(-1), lastXEnd(-1), lastZone(-1), currentBaseFreq(MIN_FREQ_BASE) {
}

void PentatonicUI::setBaseFrequency(float baseFreq) {
  currentBaseFreq = baseFreq;
}

float PentatonicUI::quantizeToPentatonic(float freq) {
  // Find which octave we're in (using current base frequency)
  float octaveNum = log(freq / currentBaseFreq) / log(2.0);
  int octave = (int)octaveNum;
  float octaveFreq = currentBaseFreq * pow(2.0, octave);
  
  // Find which pentatonic interval within this octave
  float ratio = freq / octaveFreq;
  
  // Find closest pentatonic ratio
  int closestZone = 0;
  float minDiff = fabs(ratio - PENTATONIC_RATIOS[0]);
  
  for (int i = 1; i < NUM_PENTATONIC_ZONES; i++) {
    float diff = fabs(ratio - PENTATONIC_RATIOS[i]);
    if (diff < minDiff) {
      minDiff = diff;
      closestZone = i;
    }
  }
  
  // Also check next octave's root
  if (fabs(ratio - 2.0) < minDiff) {
    return octaveFreq * 2.0;  // Next octave root
  }
  
  // Return quantized frequency
  return octaveFreq * PENTATONIC_RATIOS[closestZone];
}

int PentatonicUI::getTotalKeys(int /*screenWidth*/) {
  // Total keys are determined purely by the frequency range and pentatonic ratios
  return getTotalVisualKeys();
}

int PentatonicUI::touchToKeyIndex(int touchX, int screenWidth) {
  int totalKeys = getTotalVisualKeys();
  if (totalKeys <= 0 || screenWidth <= 0) return 0;
  // Map touch position linearly across the available keys
  int keyIndex = (touchX * totalKeys) / screenWidth;
  if (keyIndex < 0) keyIndex = 0;
  if (keyIndex >= totalKeys) keyIndex = totalKeys - 1;
  return keyIndex;
}

float PentatonicUI::keyIndexToFrequency(int keyIndex) {
  if (keyIndex < 0) keyIndex = 0;
  int totalKeys = getTotalVisualKeys();
  if (totalKeys <= 0) return currentBaseFreq;
  if (keyIndex >= totalKeys) keyIndex = totalKeys - 1;

  // Map key index to octave and pentatonic zone, then apply currentBaseFreq
  int octave = keyIndex / NUM_PENTATONIC_ZONES;
  int zone = keyIndex % NUM_PENTATONIC_ZONES;

  float octaveFreq = currentBaseFreq * pow(2.0, octave);
  return octaveFreq * PENTATONIC_RATIOS[zone];
}

float PentatonicUI::touchToFilterCutoff(int touchY, int screenHeight) {
  // Y-axis controls low-pass filter cutoff with logarithmic scaling across top half of screen
  // Top of screen (low touchY) = max cutoff, middle of screen (screenHeight/2) = min cutoff
  // Only responds to touches in the top half (0 to screenHeight/2)
  
  int controlAreaHeight = screenHeight / 2;
  
  // Constrain touchY to control area (top half)
  touchY = constrain(touchY, 0, controlAreaHeight);
  
  // Normalize: 0.0 at bottom of control area (high touchY), 1.0 at top (low touchY)
  float normalizedPos = 1.0 - ((float)touchY / (float)controlAreaHeight);
  normalizedPos = constrain(normalizedPos, 0.0, 1.0);
  
  // Logarithmic scaling: map 0.0-1.0 to filter cutoff range (human perception)
  float logMin = log10(FILTER_MIN_CUTOFF);
  float logMax = log10(FILTER_MAX_CUTOFF);
  float logRange = logMax - logMin;
  
  // Convert: normalizedPos=0.0 (bottom of control area) -> logCutoff=logMin, normalizedPos=1.0 (top) -> logCutoff=logMax
  float logCutoff = logMin + normalizedPos * logRange;
  float filterCutoff = pow(10.0, logCutoff);
  
  return constrain(filterCutoff, FILTER_MIN_CUTOFF, FILTER_MAX_CUTOFF);
}

// Helper: compute total number of visual pentatonic "keys" between MIN_FREQ and MAX_FREQ
static int getTotalVisualKeys() {
  float numOctaves = log(MAX_FREQ / MIN_FREQ) / log(2.0);
  int maxOctave = (int)numOctaves;
  int totalKeys = 0;

  for (int octave = 0; octave <= maxOctave; octave++) {
    for (int zone = 0; zone < NUM_PENTATONIC_ZONES; zone++) {
      float baseFreq = MIN_FREQ * pow(2.0, octave);
      float zoneFreq = baseFreq * PENTATONIC_RATIOS[zone];
      if (zoneFreq > MAX_FREQ) {
        break;
      }
      totalKeys++;
    }
  }
  return totalKeys;
}

void PentatonicUI::updateDisplay(TFT_eSPI& tft, bool touching, int keyIndex) {
  if (touching && keyIndex >= 0) {
    int totalKeys = getTotalVisualKeys();
    if (totalKeys <= 0) return;

    int width = tft.width();
    PentatonicZone zone;
    zone.zoneIndex = keyIndex;
    zone.xStart = (keyIndex * width) / totalKeys;
    zone.xEnd = ((keyIndex + 1) * width) / totalKeys - 1;
    if (zone.xEnd < zone.xStart) zone.xEnd = zone.xStart;
    
    // Only update if zone changed
    if (lastXStart != zone.xStart || lastXEnd != zone.xEnd || lastZone != keyIndex) {
      // Erase old highlight: fill with black and redraw dark green outline (top half only)
      int controlAreaHeight = tft.height() / 2;
      if (lastXStart >= 0 && lastXEnd >= 0) {
        tft.fillRect(lastXStart + 1, 0, lastXEnd - lastXStart - 1, 
                    controlAreaHeight, TFT_BLACK);
        // Redraw dark green outline on left edge
        if (lastXStart > 0) {
          tft.drawLine(lastXStart, 0, lastXStart, controlAreaHeight, TFT_DARKGREEN);
        }
        // Redraw dark green outline on right edge
        if (lastXEnd < tft.width() - 1) {
          tft.drawLine(lastXEnd, 0, lastXEnd, controlAreaHeight, TFT_DARKGREEN);
        }
      }
      
      // Draw new highlight: fill with yellow (top half only)
      if (zone.xEnd > zone.xStart) {
        tft.fillRect(zone.xStart + 1, 0, zone.xEnd - zone.xStart - 1, 
                    controlAreaHeight, TFT_YELLOW);
      }
      
      lastXStart = zone.xStart;
      lastXEnd = zone.xEnd;
      lastZone = keyIndex;
    }
  } else {
    // Not touching - erase highlight and restore dark green outline (top half only)
    int controlAreaHeight = tft.height() / 2;
    if (lastXStart >= 0 && lastXEnd >= 0) {
      // Fill with black
      tft.fillRect(lastXStart + 1, 0, lastXEnd - lastXStart - 1, 
                  controlAreaHeight, TFT_BLACK);
      // Redraw dark green outline on left edge
      if (lastXStart > 0) {
        tft.drawLine(lastXStart, 0, lastXStart, controlAreaHeight, TFT_DARKGREEN);
      }
      // Redraw dark green outline on right edge
      if (lastXEnd < tft.width() - 1) {
        tft.drawLine(lastXEnd, 0, lastXEnd, controlAreaHeight, TFT_DARKGREEN);
      }
      lastXStart = -1;
      lastXEnd = -1;
      lastZone = -1;
    }
  }
}

