#pragma once

#include "Map/MapData.h"

#include <memory>
#include <vector>

class TileColorCache {
public:
    struct Tile {
        const uint16_t *pixels = nullptr;
        bool isOpaque = false;
    };

    // 配置固定數量的 RGB565 瓦片快取。
    // 輸入：slotCount，快取可保存的瓦片數量。
    // 輸出：bool，記憶體是否配置成功。
    bool initialize(size_t slotCount);

    // 取得瓦片的 RGB565 快取；未命中時會由 MapData 建立。
    // 輸入：map，地圖素材資料；tileIndex，素材索引；tile，輸出快取資訊。
    // 輸出：bool，是否成功取得快取。
    bool get(const MapData &map, size_t tileIndex, Tile &tile);

    // 取得並歸零快取統計資料。
    // 輸入：無。
    // 輸出：Statistics，包含命中、未命中與未使用快取次數。
private:
    static constexpr uint16_t invalidTileIndex = 0xffff;
    static constexpr uint16_t transparentColor = 0xf81f;

    struct Slot {
        std::unique_ptr<uint16_t[]> pixels;
        uint16_t tileIndex = invalidTileIndex;
        uint32_t lastUsed = 0;
        bool isOpaque = false;
    };

    // 選出最久未使用或尚未使用的快取欄位。
    // 輸入：無。
    // 輸出：Slot*，可覆寫的快取欄位。
    Slot *selectVictim();

    std::vector<Slot> slots;
    uint32_t useCounter = 0;
};
