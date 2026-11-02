#pragma once

#include <Arduino.h>
#include <memory>
#include <vector>

// 遊戲使用期間保存在 SRAM 的地圖與共用素材。
struct MapData {
    static constexpr uint16_t tileCodeCount = 8191;
    static constexpr uint16_t invalidTileIndex = 0xffff;

    uint16_t width = 0;
    uint16_t height = 0;
    uint16_t layers = 0;
    std::vector<uint16_t> cells;
    std::vector<uint16_t> tileIds;
    // 以瓦片編號直接找到 tileIds 的索引，避免渲染時重複搜尋。
    std::unique_ptr<uint16_t[]> tileIndexByCode;
    std::vector<uint16_t> palette;
    // 每個 chunk 連續放 TILES_PER_PIXEL_CHUNK 張 tile 的色盤索引。
    std::vector<std::unique_ptr<uint8_t[]>> pixelChunks;
};
