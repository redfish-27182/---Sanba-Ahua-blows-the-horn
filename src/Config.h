#pragma once
#include <Arduino.h>

// 螢幕與 SD 卡腳位
#define TFT_BL          21
#define SD_CS_PIN       5

// 獨立觸控 SPI 腳位 (XPT2046)
#define XPT2046_IRQ     36
#define XPT2046_MOSI    32
#define XPT2046_MISO    39
#define XPT2046_CLK     25
#define XPT2046_CS      33

// 系統參數
#define SLEEP_TIMEOUT_MS  20000  // 20 秒無操作進入睡眠