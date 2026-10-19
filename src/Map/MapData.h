#pragma once

#include <Arduino.h>
#include <vector>

// 遊戲使用期間保存在 SRAM 的地圖與共用素材。
struct MapData {
    uint16_t width = 0;
    uint16_t height = 0;
    uint16_t layers = 0;
    std::vector<uint16_t> cells;
    std::vector<uint16_t> tileIds;
    std::vector<uint8_t> pixels;
};
