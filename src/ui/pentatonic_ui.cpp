/*
 * Pentatonic UI Implementation
 */

#include "pentatonic_ui.h"
#include "../config.h"
#include <math.h>

PentatonicUI::PentatonicUI() : lastXStart(-1), lastXEnd(-1), lastZone(-1) {
}

float PentatonicUI::quantizeToPentatonic(float freq) {
  // Find which octave we're in
  float octaveNum = log(freq / MIN_FREQ) / log(2.0);
  int octave = (int)octaveNum;
  float octaveFreq = MIN_FREQ * pow(2.0, octave);
  
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
  float logMin = log10(MIN_FREQ);
  float logMax = log10(MAX_FREQ);
  float logRange = logMax - logMin;
  
  // Map linear position to logarithmic frequency
  float logFreq = logMin + (xPos / 100.0) * logRange;
  float rawFreq = pow(10.0, logFreq);
  rawFreq = constrain(rawFreq, MIN_FREQ, MAX_FREQ);
  
  // Quantize to nearest pentatonic note
  return quantizeToPentatonic(rawFreq);
}

float PentatonicUI::touchToFilterCutoff(int touchY, int screenHeight) {
  // Y-axis controls low-pass filter cutoff with dead zones and logarithmic scaling
  float filterCutoff;
  
  if (touchY < DEAD_ZONE_SIZE) {
    // Top dead zone: filter open (max cutoff) - doesn't affect sound
    filterCutoff = FILTER_MAX_CUTOFF;
  } else if (touchY > screenHeight - DEAD_ZONE_SIZE) {
    // Bottom dead zone: filter closed (min cutoff) - completely kills sound
    filterCutoff = FILTER_MIN_CUTOFF;
  } else {
    // Middle area: logarithmic scaling from min (bottom) to max (top)
    float middleHeight = screenHeight - (2 * DEAD_ZONE_SIZE);
    float yPosInMiddle = touchY - DEAD_ZONE_SIZE;  // Position within middle area
    // Normalize: 0.0 at bottom of middle (high touchY), 1.0 at top of middle (low touchY)
    float normalizedPos = yPosInMiddle / middleHeight;  // 0.0 (bottom) to 1.0 (top)
    normalizedPos = constrain(normalizedPos, 0.0, 1.0);
    
    // Invert: bottom of middle (normalizedPos=0, high touchY) should be closed (min)
    //         top of middle (normalizedPos=1, low touchY) should be open (max)
    float invertedPos = 1.0 - normalizedPos;  // 1.0 at bottom, 0.0 at top
    
    // Logarithmic scaling: map 0.0-1.0 to filter cutoff range (human perception)
    float logMin = log10(FILTER_MIN_CUTOFF);
    float logMax = log10(FILTER_MAX_CUTOFF);
    float logRange = logMax - logMin;
    
    // Convert: invertedPos=1.0 (bottom) -> logCutoff=logMin, invertedPos=0.0 (top) -> logCutoff=logMax
    float logCutoff = logMin + invertedPos * logRange;
    filterCutoff = pow(10.0, logCutoff);
  }
  
  return constrain(filterCutoff, FILTER_MIN_CUTOFF, FILTER_MAX_CUTOFF);
}

int PentatonicUI::getZone(float frequency) {
  // Find which octave we're in
  float octaveNum = log(frequency / MIN_FREQ) / log(2.0);
  int octave = (int)octaveNum;
  float octaveFreq = MIN_FREQ * pow(2.0, octave);
  
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
  float octaveNum = log(frequency / MIN_FREQ) / log(2.0);
  int octave = (int)octaveNum;
  float octaveFreq = MIN_FREQ * pow(2.0, octave);
  
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
  zoneStartFreq = constrain(zoneStartFreq, MIN_FREQ, MAX_FREQ);
  zoneEndFreq = constrain(zoneEndFreq, MIN_FREQ, MAX_FREQ);
  
  // Convert to screen positions (logarithmic)
  float logMin = log10(MIN_FREQ);
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
      // Erase old highlight: fill with black and redraw dark green outline
      if (lastXStart >= 0 && lastXEnd >= 0) {
        tft.fillRect(lastXStart + 1, DEAD_ZONE_SIZE, lastXEnd - lastXStart - 1, 
                    tft.height() - (2 * DEAD_ZONE_SIZE), TFT_BLACK);
        // Redraw dark green outline on left edge
        if (lastXStart > 0) {
          tft.drawLine(lastXStart, DEAD_ZONE_SIZE, lastXStart, tft.height() - DEAD_ZONE_SIZE, TFT_DARKGREEN);
        }
        // Redraw dark green outline on right edge
        if (lastXEnd < tft.width() - 1) {
          tft.drawLine(lastXEnd, DEAD_ZONE_SIZE, lastXEnd, tft.height() - DEAD_ZONE_SIZE, TFT_DARKGREEN);
        }
      }
      
      // Draw new highlight: fill with yellow
      if (zone.xEnd > zone.xStart) {
        tft.fillRect(zone.xStart + 1, DEAD_ZONE_SIZE, zone.xEnd - zone.xStart - 1, 
                    tft.height() - (2 * DEAD_ZONE_SIZE), TFT_YELLOW);
      }
      
      lastXStart = zone.xStart;
      lastXEnd = zone.xEnd;
      lastZone = zoneIndex;
    }
  } else {
    // Not touching - erase highlight and restore dark green outline
    if (lastXStart >= 0 && lastXEnd >= 0) {
      // Fill with black
      tft.fillRect(lastXStart + 1, DEAD_ZONE_SIZE, lastXEnd - lastXStart - 1, 
                  tft.height() - (2 * DEAD_ZONE_SIZE), TFT_BLACK);
      // Redraw dark green outline on left edge
      if (lastXStart > 0) {
        tft.drawLine(lastXStart, DEAD_ZONE_SIZE, lastXStart, tft.height() - DEAD_ZONE_SIZE, TFT_DARKGREEN);
      }
      // Redraw dark green outline on right edge
      if (lastXEnd < tft.width() - 1) {
        tft.drawLine(lastXEnd, DEAD_ZONE_SIZE, lastXEnd, tft.height() - DEAD_ZONE_SIZE, TFT_DARKGREEN);
      }
      lastXStart = -1;
      lastXEnd = -1;
      lastZone = -1;
    }
  }
}

