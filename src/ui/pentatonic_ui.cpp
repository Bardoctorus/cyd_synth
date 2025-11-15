/*
 * Pentatonic UI Implementation
 */

#include "pentatonic_ui.h"
#include "../config.h"
#include <math.h>

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

float PentatonicUI::touchToFrequency(int touchX, int screenWidth) {
  // Map linear touch position to logarithmic frequency range
  float xPos = map(touchX, 0, screenWidth - 1, 0, 100);
  xPos = constrain(xPos, 0, 100);
  
  // Logarithmic scaling: convert linear 0-100 to logarithmic frequency
  // Use current base frequency instead of MIN_FREQ
  float logMin = log10(currentBaseFreq);
  float logMax = log10(MAX_FREQ);
  float logRange = logMax - logMin;
  
  // Map linear position to logarithmic frequency
  float logFreq = logMin + (xPos / 100.0) * logRange;
  float rawFreq = pow(10.0, logFreq);
  rawFreq = constrain(rawFreq, currentBaseFreq, MAX_FREQ);
  
  // Quantize to nearest pentatonic note
  return quantizeToPentatonic(rawFreq);
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

int PentatonicUI::getZone(float frequency) {
  // Find which octave we're in
  float octaveNum = log(frequency / currentBaseFreq) / log(2.0);
  int octave = (int)octaveNum;
  float octaveFreq = currentBaseFreq * pow(2.0, octave);
  
  // Find which pentatonic interval within this octave
  float ratio = frequency / octaveFreq;
  
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
    return 0;  // Next octave root
  }
  
  return closestZone;
}

PentatonicZone PentatonicUI::getZoneBounds(float frequency, int screenWidth) {
  PentatonicZone zone;
  zone.zoneIndex = getZone(frequency);
  
  // Calculate zone boundaries
  float octaveNum = log(frequency / currentBaseFreq) / log(2.0);
  int octave = (int)octaveNum;
  float octaveFreq = currentBaseFreq * pow(2.0, octave);
  
  float zoneStartFreq = octaveFreq * PENTATONIC_RATIOS[zone.zoneIndex];
  float zoneEndFreq;
  
  // Find the next pentatonic note (could be in same octave or next)
  if (zone.zoneIndex < NUM_PENTATONIC_ZONES - 1) {
    zoneEndFreq = octaveFreq * PENTATONIC_RATIOS[zone.zoneIndex + 1];
  } else {
    // Last note in octave, next is root of next octave
    zoneEndFreq = octaveFreq * 2.0;
  }
  
  // Ensure boundaries are within our range
    zoneStartFreq = constrain(zoneStartFreq, currentBaseFreq, MAX_FREQ);
    zoneEndFreq = constrain(zoneEndFreq, currentBaseFreq, MAX_FREQ);
  
  // Convert to screen positions (logarithmic)
  float logMin = log10(currentBaseFreq);
  float logMax = log10(MAX_FREQ);
  float logStart = log10(zoneStartFreq);
  float logEnd = log10(zoneEndFreq);
  zone.xStart = (int)(((logStart - logMin) / (logMax - logMin)) * screenWidth);
  zone.xEnd = (int)(((logEnd - logMin) / (logMax - logMin)) * screenWidth);
  zone.xStart = constrain(zone.xStart, 0, screenWidth - 1);
  zone.xEnd = constrain(zone.xEnd, 0, screenWidth - 1);
  
  // Ensure xEnd >= xStart
  if (zone.xEnd < zone.xStart) {
    int temp = zone.xStart;
    zone.xStart = zone.xEnd;
    zone.xEnd = temp;
  }
  
  return zone;
}

void PentatonicUI::updateDisplay(TFT_eSPI& tft, bool touching, int zoneIndex, float frequency) {
  if (touching && zoneIndex >= 0) {
    // Get zone bounds for this frequency
    PentatonicZone zone = getZoneBounds(frequency, tft.width());
    
    // Only update if zone changed
    if (lastXStart != zone.xStart || lastXEnd != zone.xEnd || lastZone != zoneIndex) {
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
      lastZone = zoneIndex;
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

