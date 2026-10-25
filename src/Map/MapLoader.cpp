#include "Map/MapLoader.h"
#include "Config.h"

#include <SD.h>
#include <cstdlib>
#include <cstring> 
#include <new>
#include <utility>

namespace {
    // 讀取 little-endian 的兩位元組整數。
    // p：const uint8_t*，指向至少兩個位元組；回傳：uint16_t，解碼結果。
    uint16_t read16(const uint8_t *p) { 
        return uint16_t(p[0]) | (uint16_t(p[1]) << 8); 
    }

    // 讀取 little-endian 的四位元組整數。
    // p：const uint8_t*，指向至少四個位元組；回傳：uint32_t，解碼結果。
    uint32_t read32(const uint8_t *p) { 
        return uint32_t(read16(p)) | (uint32_t(read16(p + 2)) << 16); 
    }
}

// 載入地圖和素材。
// mapPath、tilesetPath：const char*，檔案路徑；output：MapData&，成功時接收資料。
// 回傳：bool，是否成功。
bool MapLoader::load(const char *mapPath, const char *tilesetPath, MapData &output) {

    // 嘗試開啟地圖與素材檔案，失敗時回傳 false。
    File map   = SD.open(mapPath, FILE_READ);
    File tiles = SD.open(tilesetPath, FILE_READ);
    if (!map || !tiles) { 
        Serial.println("MapLoader: 無法開啟地圖和素材檔案"); 
        return false; 
    }

    uint8_t mh[28], th[28];
    if (map.read(mh, sizeof(mh)) != sizeof(mh) 
        || tiles.read(th, sizeof(th)) != sizeof(th) // 嘗試讀取檔案標頭，失敗時回傳 false。
        || memcmp(mh, "AHMP", 4)         // 檢查地圖檔案識別碼
        || memcmp(th, "AHTS", 4)         // 檢查素材檔案識別碼
        || read16(mh + 4) != MAP_ASSET_FORMAT_VERSION // 檢查地圖檔案版本
        || read16(th + 4) != MAP_ASSET_FORMAT_VERSION // 檢查素材檔案版本
        || read16(th + 6) != 16          // 檢查素材圖塊大小  
        || memcmp(mh + 12, th + 12, 16)  // 檢查地圖與素材的識別碼是否相符
    ) {
        Serial.println("MapLoader: 無法辨識地圖或素材檔案"); 
        return false;
    }

    // 讀取地圖尺寸、圖層數量與素材數量，檢查檔案長度與限制。
    MapData data;
    data.width  = read16(mh + 6); 
    data.height = read16(mh + 8); 
    data.layers = read16(mh + 10);
    const uint32_t count = read32(th + 8); // 讀取素材數量
    const uint64_t cellCount = uint64_t(data.width) * data.height * data.layers; // 計算地圖格子數量，避免溢位。

    if (!data.width 
        || !data.height 
        || !data.layers 
        || count > 8191 
        || cellCount > 100000 
        || map.size()   != 28 + cellCount * 2 
        || tiles.size() != 28 + uint64_t(count) * (2 + TILE_COLOR_INDEX_BYTES)
            + TILE_PALETTE_COLOR_COUNT * 2)
    {
        Serial.println("MapLoader: 地圖或素材檔案尺寸不正確"); 
        return false;
    }

    // 將色盤索引完整留在 RAM，避免每次重繪從 SD 逐張讀 tile 而造成拖曳卡頓。
    const uint64_t cellBytes = cellCount * 2;
    const uint64_t tileIdBytes = uint64_t(count) * 2;
    const uint64_t paletteBytes = TILE_PALETTE_COLOR_COUNT * 2;
    const uint64_t pixelBytes = uint64_t(count) * TILE_COLOR_INDEX_BYTES;
    const uint32_t chunkCount = (count + TILES_PER_PIXEL_CHUNK - 1) / TILES_PER_PIXEL_CHUNK;
    if (cellBytes + tileIdBytes + paletteBytes + pixelBytes + 16384 > ESP.getFreeHeap()) {
        Serial.println("MapLoader: 内存不足"); 
        return false;
    }

    if (cellBytes > ESP.getMaxAllocHeap()) {
        Serial.println("MapLoader: 記憶體碎裂導致無法載入地圖");
        return false;
    }
    data.cells.resize(size_t(cellCount));

    if (tileIdBytes > ESP.getMaxAllocHeap()) {
        Serial.println("MapLoader: 記憶體碎裂導致無法載入素材索引");
        return false;
    }
    data.tileIds.resize(count);

    data.palette.resize(TILE_PALETTE_COLOR_COUNT);

    data.pixelChunks.reserve(chunkCount);

    uint8_t pair[2];
    for (size_t i = 0; i < data.cells.size(); ++i) {
        if (map.read(pair, 2) != 2) {
            Serial.println("MapLoader: 無法讀取地圖格子");
            return false;
        }
        data.cells[i] = read16(pair);
    }
    for (size_t i = 0; i < data.tileIds.size(); ++i) {
        if (tiles.read(pair, 2) != 2) {
            Serial.println("MapLoader: 無法讀取素材索引");
            return false;
        }
        data.tileIds[i] = read16(pair);
        if (data.tileIds[i] >= 8191 || (i && data.tileIds[i] <= data.tileIds[i - 1])) return false;
    }
    for (size_t i = 0; i < data.palette.size(); ++i) {
        if (tiles.read(pair, 2) != 2) {
            Serial.println("MapLoader: 無法讀取色盤");
            return false;
        }
        data.palette[i] = uint16_t(pair[0] << 8) | pair[1];
    }
    size_t remainingPixelBytes = size_t(pixelBytes);
    for (uint32_t chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex) {
        const size_t chunkBytes = std::min(remainingPixelBytes, TILE_PIXEL_CHUNK_BYTES);
        std::unique_ptr<uint8_t[]> chunk(new (std::nothrow) uint8_t[chunkBytes]);
        if (!chunk) {
            Serial.println("MapLoader: 無法配置素材像素區塊");
            return false;
        }
        if (tiles.read(chunk.get(), chunkBytes) != chunkBytes) {
            Serial.println("MapLoader: 無法讀取素材像素區塊");
            return false;
        }
        data.pixelChunks.push_back(std::move(chunk));
        remainingPixelBytes -= chunkBytes;
    }
    
    for (uint16_t cell : data.cells) {
        const uint16_t code = cell & 0x1fff;
        if (!code) { if (cell) return false; continue; }
        bool found = false;
        for (uint16_t id : data.tileIds) if (id == code - 1) { found = true; break; }
        if (!found) { Serial.println("MapLoader: missing tile"); return false; }
    }
    output = std::move(data);
    Serial.printf("MapLoader: %u x %u, %u layers, %u tiles loaded\n", output.width, output.height, output.layers, count);
    return true;
}
