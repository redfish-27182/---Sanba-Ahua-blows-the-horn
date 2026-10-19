#include "Map/MapLoader.h"

#include <SD.h>
#include <cstring>
#include <utility>

namespace {
// 讀取 little-endian 的兩位元組整數。
// p：const uint8_t*，指向至少兩個位元組；回傳：uint16_t，解碼結果。
uint16_t read16(const uint8_t *p) { return uint16_t(p[0]) | (uint16_t(p[1]) << 8); }

// 讀取 little-endian 的四位元組整數。
// p：const uint8_t*，指向至少四個位元組；回傳：uint32_t，解碼結果。
uint32_t read32(const uint8_t *p) { return uint32_t(read16(p)) | (uint32_t(read16(p + 2)) << 16); }
}

// 載入轉檔工具版本 2 的地圖和素材，檢查長度、索引及素材庫識別碼。
// mapPath、tilesetPath：const char*，檔案路徑；output：MapData&，成功時接收資料。
// 回傳：bool，是否成功。
bool MapLoader::load(const char *mapPath, const char *tilesetPath, MapData &output) {
    File map = SD.open(mapPath, FILE_READ);
    File tiles = SD.open(tilesetPath, FILE_READ);
    if (!map || !tiles) { Serial.println("MapLoader: cannot open map or tileset"); return false; }
    uint8_t mh[28], th[28];
    if (map.read(mh, sizeof(mh)) != sizeof(mh) || tiles.read(th, sizeof(th)) != sizeof(th) ||
        memcmp(mh, "AHMP", 4) || memcmp(th, "AHTS", 4) || read16(mh + 4) != 2 ||
        read16(th + 4) != 2 || read16(th + 6) != 16 || memcmp(mh + 12, th + 12, 16)) {
        Serial.println("MapLoader: invalid header or mismatched tileset"); return false;
    }
    MapData data;
    data.width = read16(mh + 6); data.height = read16(mh + 8); data.layers = read16(mh + 10);
    const uint32_t count = read32(th + 8);
    const uint64_t cellCount = uint64_t(data.width) * data.height * data.layers;
    if (!data.width || !data.height || !data.layers || count > 8191 ||
        cellCount > 100000 || map.size() != 28 + cellCount * 2 ||
        tiles.size() != 28 + uint64_t(count) * 514) {
        Serial.println("MapLoader: invalid dimensions or file length"); return false;
    }
    // 限制分配量，並檢查連續記憶體是否足夠；不配置完整螢幕或大地圖。
    if (cellCount * 2 + uint64_t(count) * 514 + 16384 > ESP.getFreeHeap()) {
        Serial.println("MapLoader: insufficient heap"); return false;
    }
    if (cellCount * 2 > ESP.getMaxAllocHeap()) { Serial.println("MapLoader: fragmented heap for map"); return false; }
    data.cells.resize(size_t(cellCount));
    if (uint64_t(count) * 2 > ESP.getMaxAllocHeap()) { Serial.println("MapLoader: fragmented heap for index"); return false; }
    data.tileIds.resize(count);
    if (uint64_t(count) * 512 > ESP.getMaxAllocHeap()) { Serial.println("MapLoader: fragmented heap for pixels"); return false; }
    data.pixels.resize(size_t(count) * 512);
    uint8_t pair[2];
    for (size_t i = 0; i < data.cells.size(); ++i) {
        if (map.read(pair, 2) != 2) return false;
        data.cells[i] = read16(pair);
    }
    for (size_t i = 0; i < data.tileIds.size(); ++i) {
        if (tiles.read(pair, 2) != 2) return false;
        data.tileIds[i] = read16(pair);
        if (data.tileIds[i] >= 8191 || (i && data.tileIds[i] <= data.tileIds[i - 1])) return false;
    }
    if (!data.pixels.empty() && tiles.read(data.pixels.data(), data.pixels.size()) != data.pixels.size()) return false;
    for (uint16_t cell : data.cells) {
        const uint16_t code = cell & 0x1fff;
        if (!code) { if (cell) return false; continue; }
        bool found = false;
        for (uint16_t id : data.tileIds) if (id == code - 1) { found = true; break; }
        if (!found) { Serial.println("MapLoader: missing tile"); return false; }
    }
    output = std::move(data);
    Serial.printf("MapLoader: %u x %u, %u layers, %u tiles; free heap %u\n", output.width, output.height, output.layers, count, ESP.getFreeHeap());
    return true;
}
