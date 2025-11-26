/*
 * Menu System
 * Handles menu button and menu screen display
 */

#ifndef MENU_H
#define MENU_H

#include <TFT_eSPI.h>

enum MenuState {
  MENU_CLOSED,    // Menu is closed, showing main interface
  MENU_OPEN       // Menu is open, showing menu screen
};

class Menu {
public:
  Menu();
  
  // Initialize menu (draw button)
  void init(TFT_eSPI& tft);
  
  // Check if touch is on menu button
  bool isMenuButtonTouched(int touchX, int touchY, int screenWidth, int screenHeight);
  
  // Check if touch is on panic button (above menu button)
  bool isPanicButtonTouched(int touchX, int touchY, int screenWidth, int screenHeight);
  
  // Open menu (show menu screen)
  void open(TFT_eSPI& tft);
  
  // Close menu (return to main interface)
  void close(TFT_eSPI& tft);
  
  // Get current menu state
  MenuState getState() { return state; }
  
  // Draw menu button (call from display update)
  void drawButton(TFT_eSPI& tft);
  
  // Draw panic button (call from display update)
  void drawPanicButton(TFT_eSPI& tft);
  
private:
  MenuState state;
  int buttonX, buttonY, buttonWidth, buttonHeight;
  int panicX, panicY, panicWidth, panicHeight;
  void drawMenuScreen(TFT_eSPI& tft);
};

#endif // MENU_H

