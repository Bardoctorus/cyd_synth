/*
 * Display Hardware Implementation
 */

#include "display.h"
#include "../config.h"

Display::Display() : lastStaticRedraw(0) {
}

void Display::init() {
  tft.init();
  tft.setRotation(1);  // Landscape mode
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  
  // Draw initial static elements
  drawDeadZones();
  drawPentatonicZones();
}

void Display::drawDeadZones() {
  // Draw red rectangles for top and bottom dead zones
  tft.drawRect(0, 0, tft.width(), DEAD_ZONE_SIZE, TFT_RED);
  tft.drawRect(0, tft.height() - DEAD_ZONE_SIZE, tft.width(), DEAD_ZONE_SIZE, TFT_RED);
}

void Display::drawPentatonicZones() {
  // Calculate how many octaves fit in our frequency range
  float numOctaves = log(MAX_FREQ / MIN_FREQ) / log(2.0);
  
  // Draw vertical lines for each pentatonic zone boundary (dark green outlines)
  for (int octave = 0; octave <= (int)numOctaves; octave++) {
    for (int zone = 0; zone < NUM_PENTATONIC_ZONES; zone++) {
      // Calculate frequency for this zone
      float baseFreq = MIN_FREQ * pow(2.0, octave);
      float zoneFreq = baseFreq * PENTATONIC_RATIOS[zone];
      
      if (zoneFreq > MAX_FREQ) break;
      
      // Convert frequency to screen X position (logarithmic)
      float logMin = log10(MIN_FREQ);
      float logMax = log10(MAX_FREQ);
      float logFreq = log10(zoneFreq);
      float normalizedPos = (logFreq - logMin) / (logMax - logMin);
      int xPos = (int)(normalizedPos * tft.width());
      
      if (xPos >= 0 && xPos < tft.width()) {
        // Draw vertical line (dark green outline, only in the playable area, avoiding dead zones)
        tft.drawLine(xPos, DEAD_ZONE_SIZE, xPos, tft.height() - DEAD_ZONE_SIZE, TFT_DARKGREEN);
      }
    }
  }
}

void Display::redrawStaticElements() {
  // Redraw dead zones
  drawDeadZones();
  
  // Redraw pentatonic zone dividers
  drawPentatonicZones();
  
  // Redraw text that might get rubbed off
  tft.setCursor(10, 100);
  tft.setTextColor(TFT_GREEN);
  tft.println("I2S: OK");
  tft.setCursor(10, 120);
  tft.setTextColor(TFT_GREEN);
  tft.println("Audio: RUNNING");
}

void Display::update() {
  // Redraw static elements periodically to prevent "rubbing out"
  unsigned long currentTime = millis();
  if (currentTime - lastStaticRedraw >= (unsigned long)STATIC_REDRAW_INTERVAL) {
    lastStaticRedraw = currentTime;
    redrawStaticElements();
  }
}

