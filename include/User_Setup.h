// User_Setup.h for TFT_eSPI library
// Configure this file for your specific CYD board display

// Driver selection - uncomment the driver for your display
#define ILI9341_DRIVER
// #define ST7789_DRIVER  // Alternative if your board uses ST7789

// Display pins - adjust these for your CYD board
#define TFT_MOSI  23
#define TFT_SCLK  18
#define TFT_CS    15
#define TFT_DC    2
#define TFT_RST   4
#define TFT_BL    14  // Backlight pin (optional, comment out if not used)

// Display settings
#define TFT_WIDTH  240
#define TFT_HEIGHT 320
#define TFT_INVERSION_ON

// SPI settings
#define TFT_SPI_FREQUENCY  27000000
#define TFT_SPI_READ_FREQUENCY  20000000
#define TFT_SPI_FACTOR 64

// Colors (usually don't need to change)
#define TFT_RED      0xF800
#define TFT_GREEN    0x07E0
#define TFT_BLUE     0x001F
#define TFT_BLACK    0x0000
#define TFT_WHITE    0xFFFF
#define TFT_YELLOW   0xFFE0
#define TFT_CYAN     0x07FF
#define TFT_MAGENTA  0xF81F

// Optional: Enable ESP32 specific optimizations
#define ESP32_PARALLEL

// Note: If your display doesn't work, try:
// 1. Changing the driver (ILI9341 vs ST7789)
// 2. Adjusting pin numbers
// 3. Changing TFT_INVERSION_ON to TFT_INVERSION_OFF
// 4. Adjusting SPI frequency (try lower values like 20000000)

