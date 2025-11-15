/*
 * Touchscreen Hardware Implementation
 */

#include "touch.h"
#include "../config.h"

TouchScreen::TouchScreen() : touchscreenSPI(VSPI), ts(XPT2046_CS, XPT2046_IRQ) {
}

void TouchScreen::init() {
  // Start separate SPI bus for touchscreen with explicit pins
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  
  // Initialize touchscreen with the SPI instance
  ts.begin(touchscreenSPI);
  ts.setRotation(1);  // Match display rotation (landscape mode)
}

TouchData TouchScreen::read(int screenWidth, int screenHeight) {
  TouchData data;
  data.active = false;
  data.x = 0;
  data.y = 0;
  
  if (ts.tirqTouched() && ts.touched()) {
    TS_Point p = ts.getPoint();
    
    // Convert touch coordinates to display coordinates
    // Calibration values from Random Nerd Tutorials example for CYD board
    data.x = map(p.x, 200, 3700, 1, screenWidth);
    data.y = map(p.y, 240, 3800, 1, screenHeight);
    
    // Constrain to display bounds
    data.x = constrain(data.x, 0, screenWidth - 1);
    data.y = constrain(data.y, 0, screenHeight - 1);
    data.active = true;
  }
  
  return data;
}

