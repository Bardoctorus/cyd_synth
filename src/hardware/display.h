/*
 * Display Hardware Abstraction
 * Handles TFT display initialization, drawing, and refresh management
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <TFT_eSPI.h>

class Display {
public:
  Display();
  void init();
  void drawPentatonicZones();
  void redrawStaticElements();  // Redraws pentatonic dividers and text
  void update();  // Periodic update (call from loop)
  
  // Access to underlying TFT for advanced operations
  TFT_eSPI& getTFT() { return tft; }
  
private:
  TFT_eSPI tft;
  unsigned long lastStaticRedraw;
};

#endif // DISPLAY_H

