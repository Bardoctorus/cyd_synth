/*
 * Menu System
 * Handles menu button and menu screen display
 */

#include "menu.h"
#include "../config.h"

Menu::Menu() : state(MENU_CLOSED) {
}

void Menu::init(TFT_eSPI& tft) {
  // Menu button in bottom left corner
  // Button size: thin vertical strip, MENU_BUTTON_WIDTH x 30 pixels
  buttonWidth = MENU_BUTTON_WIDTH;
  buttonHeight = 30;
  buttonX = 0;
  buttonY = tft.height() - buttonHeight;  // Flush with bottom edge
  
  // Panic button directly above menu button (same width and height)
  panicWidth = buttonWidth;
  panicHeight = buttonHeight;
  panicX = buttonX;
  panicY = buttonY - panicHeight - 1;  // 1px gap above blue button
  
  drawButton(tft);
  drawPanicButton(tft);
}

bool Menu::isMenuButtonTouched(int touchX, int touchY, int screenWidth, int screenHeight) {
  // Check if touch is within menu button bounds
  return (touchX >= buttonX && touchX < (buttonX + buttonWidth) &&
          touchY >= buttonY && touchY < (buttonY + buttonHeight));
}

bool Menu::isPanicButtonTouched(int touchX, int touchY, int screenWidth, int screenHeight) {
  // Check if touch is within panic button bounds
  return (touchX >= panicX && touchX < (panicX + panicWidth) &&
          touchY >= panicY && touchY < (panicY + panicHeight));
}

void Menu::open(TFT_eSPI& tft) {
  state = MENU_OPEN;
  drawMenuScreen(tft);
}

void Menu::close(TFT_eSPI& tft) {
  state = MENU_CLOSED;
  // Clear menu screen area (full screen)
  tft.fillScreen(TFT_BLACK);
  // Redraw will happen in main loop via display.update()
}

void Menu::drawButton(TFT_eSPI& tft) {
  // Draw menu button as a solid blue strip at bottom left
  tft.fillRect(buttonX, buttonY, buttonWidth, buttonHeight, TFT_BLUE);
  // Draw a white dividing line on the right edge (visual separator from faders)
  int lineX = buttonX + buttonWidth - 1;
  int screenHeight = tft.height();
  tft.drawLine(lineX, tft.height() / 2, lineX, screenHeight - 1, TFT_WHITE);
}

void Menu::drawPanicButton(TFT_eSPI& tft) {
  // Draw panic button as a solid red strip directly above the menu button
  tft.fillRect(panicX, panicY, panicWidth, panicHeight, TFT_RED);
}

void Menu::drawMenuScreen(TFT_eSPI& tft) {
  // Clear screen
  tft.fillScreen(TFT_BLACK);
  
  // Draw menu title
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print("MENU");
  
  // Draw placeholder text
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(2);
  int centerX = tft.width() / 2;
  int centerY = tft.height() / 2;
  int textWidth = 20 * 6;  // Approximate width of text (20 chars * 6 pixels per char)
  tft.setCursor(centerX - textWidth / 2, centerY);
  tft.print("MENU WILL BE HERE");
  
  // Draw close button (back button in top left)
  tft.fillRoundRect(5, 5, 50, 30, 3, TFT_DARKGREY);
  tft.drawRoundRect(5, 5, 50, 30, 3, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setTextSize(1);
  tft.setCursor(12, 12);
  tft.print("BACK");
}

