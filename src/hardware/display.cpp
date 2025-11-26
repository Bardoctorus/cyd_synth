/*
 * Display Hardware Implementation
 */

#include "display.h"
#include "../config.h"

Display::Display() : lastStaticRedraw(0) {
  // Menu button geometry is kept in sync with Menu class
  // Thin vertical strip at bottom left
  menuButtonWidth = MENU_BUTTON_WIDTH;
  menuButtonHeight = 30;
}

void Display::init() {
  tft.init();
  tft.setRotation(1);  // Landscape mode
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  
  // Menu button position: very bottom left
  menuButtonX = 0;
  menuButtonY = tft.height() - menuButtonHeight;
  
  // Draw initial static elements
  drawPentatonicZones();
}

void Display::drawPentatonicZones() {
  // We want "piano key" style equal-width zones visually, even though the
  // underlying frequencies are spaced logarithmically. To do this we:
  // 1) Count how many pentatonic notes fit between MIN_FREQ and MAX_FREQ
  // 2) Give each note an equal share of the horizontal pixel range

  // Step 1: count total visual keys
  int totalKeys = 0;
  float numOctaves = log(MAX_FREQ / MIN_FREQ) / log(2.0);
  int maxOctave = (int)numOctaves;

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

  if (totalKeys <= 0) return;

  // Step 2: draw a vertical line at the start of each key region
  int width = tft.width();
  int controlAreaHeight = tft.height() / 2;
  int keyIndex = 0;

  for (int octave = 0; octave <= maxOctave; octave++) {
    for (int zone = 0; zone < NUM_PENTATONIC_ZONES; zone++) {
      float baseFreq = MIN_FREQ * pow(2.0, octave);
      float zoneFreq = baseFreq * PENTATONIC_RATIOS[zone];
      if (zoneFreq > MAX_FREQ) {
        break;
      }

      int xPos = (keyIndex * width) / totalKeys;
      if (xPos >= 0 && xPos < width) {
        tft.drawLine(xPos, 0, xPos, controlAreaHeight, TFT_DARKGREEN);
      }

      keyIndex++;
    }
  }
}

void Display::redrawStaticElements() {
  // Redraw pentatonic zone dividers
  drawPentatonicZones();
  
  // Redraw divider line between top and bottom halves
  int controlAreaHeight = tft.height() / 2;
  tft.drawLine(0, controlAreaHeight, tft.width(), controlAreaHeight, TFT_WHITE);
  // Note: Menu button is redrawn separately by Menu class
}

bool Display::isInMenuButtonArea(int x, int y) {
  return (x >= menuButtonX && x < (menuButtonX + menuButtonWidth) &&
          y >= menuButtonY && y < (menuButtonY + menuButtonHeight));
}

void Display::update() {
  // Redraw static elements periodically to prevent "rubbing out"
  unsigned long currentTime = millis();
  if (currentTime - lastStaticRedraw >= (unsigned long)STATIC_REDRAW_INTERVAL) {
    lastStaticRedraw = currentTime;
    redrawStaticElements();
  }
}

