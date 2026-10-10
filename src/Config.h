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

// OTA 相關設定
constexpr char CURRENT_FIRMWARE_VERSION[] = "0.1.1"; // 目前的韌體版本號
constexpr char GITHUB_RELEASE_BASE_URL[] = "https://github.com/redfish-27182/---Sanba-Ahua-blows-the-horn/releases/download/";
constexpr char GITHUB_RELEASE_TAG_PREFIX[] = "v"; // GitHub Release 的標籤前綴
constexpr char GITHUB_FIRMWARE_FILENAME[] = "firmware.bin";

// 系統參數
#define SLEEP_TIMEOUT_MS  20000  // 20 秒無操作進入睡眠
