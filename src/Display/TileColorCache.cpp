#include "Display/TileColorCache.h"

#include "Config.h"

#include <new>

// 配置每一張瓦片獨立的記憶體區塊，降低 Heap 連續空間不足的風險。
// 輸入：slotCount，快取可保存的瓦片數量。
// 輸出：bool，記憶體是否配置成功。
bool TileColorCache::initialize(size_t slotCount) {
    if (!slots.empty()) return true;

    slots.reserve(slotCount);
    for (size_t index = 0; index < slotCount; ++index) {
        Slot slot{};
        slot.pixels.reset(new (std::nothrow) uint16_t[TILE_PIXEL_COUNT]);
        if (!slot.pixels) {
            slots.clear();
            Serial.println("[TileColorCache] 快取記憶體不足，使用原始渲染方式");
            return false;
        }
        slots.push_back(std::move(slot));
    }

    Serial.printf(
        "[TileColorCache] 已配置 %u 張 RGB565 快取瓦片（%u bytes）\n",
        static_cast<unsigned>(slots.size()),
        static_cast<unsigned>(slots.size() * TILE_PIXEL_COUNT * sizeof(uint16_t))
    );
    return true;
}

// 先使用尚未填入素材的欄位；沒有空位時選出最久未使用者。
// 輸入：無。
// 輸出：Slot*，可覆寫的快取欄位。
TileColorCache::Slot *TileColorCache::selectVictim() {
    Slot *victim = nullptr;
    for (Slot &slot : slots) {
        if (slot.tileIndex == invalidTileIndex) return &slot;
        if (victim == nullptr || slot.lastUsed < victim->lastUsed) {
            victim = &slot;
        }
    }
    return victim;
}

// 找出或建立指定瓦片的 RGB565 資料，並記錄是否完全不透明。
// 輸入：map，地圖素材資料；tileIndex，素材索引；tile，輸出快取資訊。
// 輸出：bool，是否成功取得快取。
bool TileColorCache::get(const MapData &map, size_t tileIndex, Tile &tile) {
    tile = {};
    if (slots.empty() || tileIndex >= map.tileIds.size()) return false;

    ++useCounter;
    if (useCounter == 0) {
        useCounter = 1;
        for (Slot &slot : slots) slot.lastUsed = 0;
    }

    for (Slot &slot : slots) {
        if (slot.tileIndex == tileIndex) {
            slot.lastUsed = useCounter;
            tile.pixels = slot.pixels.get();
            tile.isOpaque = slot.isOpaque;
            return true;
        }
    }

    const size_t chunkIndex = tileIndex / TILES_PER_PIXEL_CHUNK;
    const size_t tileInChunk = tileIndex % TILES_PER_PIXEL_CHUNK;
    if (chunkIndex >= map.pixelChunks.size() || !map.pixelChunks[chunkIndex]) return false;

    Slot *slot = selectVictim();
    if (slot == nullptr || !slot->pixels) return false;

    const uint8_t *source = map.pixelChunks[chunkIndex].get()
        + tileInChunk * TILE_COLOR_INDEX_BYTES;
    bool isOpaque = true;
    for (size_t pixel = 0; pixel < TILE_PIXEL_COUNT; ++pixel) {
        const uint8_t paletteIndex = source[pixel];
        if (paletteIndex >= map.palette.size()) return false;

        const uint16_t color = map.palette[paletteIndex];
        slot->pixels[pixel] = color;
        if (color == transparentColor) isOpaque = false;
    }

    slot->tileIndex = static_cast<uint16_t>(tileIndex);
    slot->isOpaque = isOpaque;
    slot->lastUsed = useCounter;
    tile.pixels = slot->pixels.get();
    tile.isOpaque = slot->isOpaque;
    return true;
}

// 讀出這段期間的快取使用情況，並開始下一段統計。
// 輸入：無。
// 輸出：Statistics，包含累積的快取計數。
