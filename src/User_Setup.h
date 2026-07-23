#ifndef USER_SETUP_H
#define USER_SETUP_H

// 🎯 1. 驅動晶片設定 (CYD 小黃板標準 ILI9341 / ST7789)
#define ILI9341_2_DRIVER     // 專為小黃板優化的 ILI9341 驅動

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// 🎯 2. 小黃板 (CYD) 專用 SPI 腳位
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15  // Chip select control pin
#define TFT_DC    2  // Data Command control pin
#define TFT_RST  -1  // Set TFT_RST to -1 if display RESET is connected to ESP32 board RST
#define TFT_BL   21  // LED back-light control pin

// 🎯 3. 觸控晶片 (XPT2046) 腳位
#define TOUCH_CS 33  // Chip select pin (T_CS) of touch screen

// 🎯 4. 字型啟用
#define LOAD_GLCD    // Font 1. Original Adafruit 8 pixel font
#define LOAD_FONT2   // Font 2. Small 16 pixel font
#define LOAD_FONT4   // Font 4. Medium 26 pixel font
#define LOAD_FONT6   // Font 6. Large 48 pixel font
#define LOAD_FONT7   // Font 7. 7 segment 48 pixel font
#define LOAD_FONT8   // Font 8. Large 75 pixel font
#define LOAD_GFXFF   // FreeFonts. Include access to the 48 Adafruit_GFX free fonts

#define SMOOTH_FONT

// 🎯 5. SPI 匯流排速度
#define SPI_FREQUENCY  55000000 // 55MHz (流暢不卡頓)
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000 // 觸控晶片通訊頻率

#endif