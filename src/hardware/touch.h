/*
 * Touchscreen Hardware Abstraction
 * Handles touchscreen initialization and reading
 */

#ifndef TOUCH_H
#define TOUCH_H

#include <XPT2046_Touchscreen.h>
#include <SPI.h>

struct TouchData {
  bool active;      // Is screen currently being touched?
  int x;            // Touch X coordinate (0 to screen width)
  int y;            // Touch Y coordinate (0 to screen height)
};

class TouchScreen {
public:
  TouchScreen();
  void init();
  TouchData read(int screenWidth, int screenHeight);  // Read current touch state
  
private:
  XPT2046_Touchscreen ts;
  SPIClass touchscreenSPI;
};

#endif // TOUCH_H

