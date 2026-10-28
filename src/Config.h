#pragma once
#include <Arduino.h>

// 螢幕旋轉後的顯示尺寸；需與 TFT 驅動設定及旋轉方向一致。
constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 240;

// 與 tools/map_converter/common.py 的 FORMAT_VERSION 必須一致。
constexpr uint16_t MAP_ASSET_FORMAT_VERSION = 3;

// 每個 16×16 tile 用 8-bit 色盤索引表示；色盤本身仍是 RGB565，畫質不變。
constexpr size_t TILE_PIXEL_COUNT = 16 * 16;
constexpr size_t TILE_COLOR_INDEX_BYTES = TILE_PIXEL_COUNT;
constexpr size_t TILE_PALETTE_COLOR_COUNT = 256;
constexpr size_t TILES_PER_PIXEL_CHUNK = 16;
constexpr size_t TILE_PIXEL_CHUNK_BYTES = TILE_COLOR_INDEX_BYTES * TILES_PER_PIXEL_CHUNK;

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
// 電源管理：先關閉背光，長時間沒有操作才進入 deep sleep。
constexpr uint32_t DISPLAY_SLEEP_TIMEOUT_MS = 40000;        // 40 秒
constexpr uint32_t DEEP_SLEEP_TIMEOUT_MS = 5 * 60 * 1000;   // 5 分鐘
