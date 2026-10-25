#pragma once

#include <Arduino.h>
#include <memory>
#include <vector>

// 遊戲使用期間保存在 SRAM 的地圖與共用素材。
struct MapData {
    uint16_t width = 0;
    uint16_t height = 0;
    uint16_t layers = 0;
    std::vector<uint16_t> cells;
    std::vector<uint16_t> tileIds;
    std::vector<uint16_t> palette;
    // 每個 chunk 連續放 TILES_PER_PIXEL_CHUNK 張 tile 的色盤索引。
    std::vector<std::unique_ptr<uint8_t[]>> pixelChunks;
};
