#include "Display/TileRenderer.h"
#include <algorithm>
#include <cstring>
#include <new>

// 配置四行 RGB565 緩衝區，保留到渲染器結束。
// 無輸入；回傳：bool，是否成功配置。
bool TileRenderer::initialize() {
    // 若已配置則直接回傳成功。
    if (strip && nextStrip) return true;
    
    strip.reset(new (std::nothrow) uint16_t[stripWidth * stripHeight]);
    nextStrip.reset(new (std::nothrow) uint16_t[stripWidth * stripHeight]);
    
    if (!strip || !nextStrip) {
        strip.reset();
        nextStrip.reset();
        Serial.println("[TileRenderer] double strip allocation failed");
        return false;
    }

    // 快取配置失敗時仍保留原始渲染路徑，避免因最佳化而無法進入遊戲。
    // 輸入：24，最多保存 24 張 16×16 RGB565 瓦片。
    tileColorCache.initialize(24);

    return true;
}

// 按圖層合成可見像素，桃紅色為透明；tile 像素已常駐於 RAM。
bool TileRenderer::draw(
    const MapData &map, 
    const Camera &camera, 
    const ScreenOverlay &overlay,
    DisplayDriver &display
) {
    if (!strip || !nextStrip) {
        Serial.println("[TileRenderer] 還沒初始化ˇ");
        return false;
    }
    const int cameraDeltaX = camera.x - lastCameraX;
    const int cameraDeltaY = camera.y - lastCameraY;
    const int absoluteDeltaX = cameraDeltaX < 0 ? -cameraDeltaX : cameraDeltaX;
    const int absoluteDeltaY = cameraDeltaY < 0 ? -cameraDeltaY : cameraDeltaY;
    // 鏡頭向下看時，新地圖內容從畫面底部進入；優先更新底部可減少撕裂感。
    const bool drawBottomFirst = hasLastCameraPosition &&
        absoluteDeltaY > absoluteDeltaX && cameraDeltaY > 0;
    const int stripCount = (SCREEN_HEIGHT + stripHeight - 1) / stripHeight;
    display.beginFrame();
    for (int stripIndex = 0; stripIndex < stripCount; ++stripIndex) {
        uint16_t *activeStrip = (stripIndex & 1) ? nextStrip.get() : strip.get();
        const int sy = drawBottomFirst
            ? std::max(0, SCREEN_HEIGHT - (stripIndex + 1) * stripHeight)
            : stripIndex * stripHeight;
        const int currentHeight = std::min(stripHeight, SCREEN_HEIGHT - sy);
        // 先清空緩衝區
        std::fill_n(activeStrip, stripWidth * stripHeight, uint16_t(0));
        
        // 依圖層順序合成可見像素
        for (unsigned layer = 0; layer < map.layers; ++layer) {

            // 每個區塊不跨越瓦片邊界，因此只需查找一次素材。
            for (int py = 0; py < currentHeight;) {
                const int wy = camera.y + sy + py;
                if (wy >= int(map.height) * 16) break;

                const int blockHeight = std::min(currentHeight - py, 16 - wy % 16);
                for (int px = 0; px < stripWidth;) {
                    const int wx = camera.x + px;
                    if (wx >= int(map.width) * 16) break;
                    const int blockWidth = std::min(stripWidth - px, 16 - wx % 16);
                    const size_t pos = (size_t(layer) * map.height + wy / 16) * map.width + wx / 16;
                    const uint16_t cell = map.cells[pos], code = cell & 0x1fff;
                    if (code) {
                        const uint16_t tileIndexValue = map.tileIndexByCode[code - 1];
                        if (tileIndexValue != MapData::invalidTileIndex) {
                            const size_t tileIndex = tileIndexValue;

                            TileColorCache::Tile cachedTile{};
                            if (tileColorCache.get(map, tileIndex, cachedTile)) {
                                const int tileX = wx % 16;
                                const int tileY = wy % 16;
                                const uint16_t transformFlags = cell & 0xe000;

                                // 未翻轉且不透明的瓦片可直接複製 RGB565 像素。
                                // 輸入：cachedTile、activeStrip、目前瓦片區塊座標。
                                if (transformFlags == 0) {
                                    for (int dy = 0; dy < blockHeight; ++dy) {
                                        const uint16_t *source = cachedTile.pixels
                                            + (tileY + dy) * 16 + tileX;
                                        uint16_t *target = activeStrip
                                            + (py + dy) * stripWidth + px;
                                        if (cachedTile.isOpaque) {
                                            memcpy(target, source, blockWidth * sizeof(uint16_t));
                                        } else {
                                            for (int dx = 0; dx < blockWidth; ++dx) {
                                                const uint16_t color = source[dx];
                                                if (color != 0xf81f) target[dx] = color;
                                            }
                                        }
                                    }
                                } else {
                                    // 有翻轉旗標時仍使用通用座標轉換，但省略 palette 查詢。
                                    // 輸入：cell 翻轉旗標、cachedTile RGB565 像素、目前瓦片區塊座標。
                                    for (int dy = 0; dy < blockHeight; ++dy) {
                                        for (int dx = 0; dx < blockWidth; ++dx) {
                                            int sourceX = tileX + dx;
                                            int sourceY = tileY + dy;
                                            if (cell & 0x2000) sourceX = 15 - sourceX;
                                            if (cell & 0x4000) sourceY = 15 - sourceY;
                                            if (cell & 0x8000) {
                                                const int temporary = sourceX;
                                                sourceX = sourceY;
                                                sourceY = temporary;
                                            }
                                            const uint16_t color = cachedTile.pixels[sourceY * 16 + sourceX];
                                            if (color != 0xf81f) {
                                                activeStrip[(py + dy) * stripWidth + px + dx] = color;
                                            }
                                        }
                                    }
                                }
                                px += blockWidth;
                                continue;
                            }

                            const size_t chunkIndex = tileIndex / TILES_PER_PIXEL_CHUNK;
                            const size_t tileInChunk = tileIndex % TILES_PER_PIXEL_CHUNK;
                            if (chunkIndex >= map.pixelChunks.size() || !map.pixelChunks[chunkIndex]) {
                                Serial.println("[TileRenderer] tile pixel chunk is unavailable");
                                display.endFrame();
                                return false;
                            }
                            const uint8_t *tilePixels = map.pixelChunks[chunkIndex].get()
                                + tileInChunk * TILE_COLOR_INDEX_BYTES;
                            for (int dy = 0; dy < blockHeight; ++dy) {
                                for (int dx = 0; dx < blockWidth; ++dx) {
                                    int x = wx % 16 + dx, y = wy % 16 + dy;
                                    // 反向查詢來源像素：先反轉水平、垂直，再反轉對角線。
                                    if (cell & 0x2000) x = 15 - x;
                                    if (cell & 0x4000) y = 15 - y;
                                    if (cell & 0x8000) { const int temp = x; x = y; y = temp; }
                                    const uint8_t paletteIndex = tilePixels[y * 16 + x];
                                    if (paletteIndex >= map.palette.size()) {
                                        Serial.println("[TileRenderer] invalid tile palette index");
                                        display.endFrame();
                                        return false;
                                    }
                                    const uint16_t color = map.palette[paletteIndex];
                                    if (color != 0xf81f) activeStrip[(py + dy) * stripWidth + px + dx] = color;
                                }
                            }
                        }
                    }
                    px += blockWidth;
                }
                py += blockHeight;
            }
        }
        // 在同一個條帶依序疊加 HUD 等固定介面，再一次送往 TFT。
        // pixels：uint16_t*，目前條帶；sy、currentHeight：int，條帶螢幕位置與高度。
        overlay.composeStrip(activeStrip, sy, currentHeight);
        display.drawStrip(sy, currentHeight, activeStrip);
        yield();
    }
    display.endFrame();

#if 0 // 已停用的效能統計：保留原始碼供日後除錯時參考。
    // 每 20 幀列印一次平均值，避免 Serial 輸出干擾平常的拖曳流暢度。
    // 輸入：本幀的合成、傳輸與總耗時，單位為微秒。
    static uint32_t measuredFrameCount = 0;
    static uint32_t accumulatedComposeUs = 0;
    static uint32_t accumulatedTransferUs = 0;
    static uint32_t accumulatedFrameUs = 0;
    ++measuredFrameCount;
    accumulatedComposeUs += composeTimeUs;
    accumulatedTransferUs += transferTimeUs;
    accumulatedFrameUs += micros() - frameStartedAt;
    const TileColorCache::Statistics cacheStatistics = tileColorCache.takeStatistics();
    static uint32_t accumulatedCacheHits = 0;
    static uint32_t accumulatedCacheMisses = 0;
    static uint32_t accumulatedCacheBypasses = 0;
    static uint32_t accumulatedOpaqueCopyBlocks = 0;
    static uint32_t accumulatedTransparentCopyBlocks = 0;
    static uint32_t accumulatedTransformedBlocks = 0;
    static uint32_t accumulatedFallbackBlocks = 0;
    accumulatedCacheHits += cacheStatistics.hits;
    accumulatedCacheMisses += cacheStatistics.misses;
    accumulatedCacheBypasses += cacheStatistics.bypasses;
    accumulatedOpaqueCopyBlocks += opaqueCopyBlocks;
    accumulatedTransparentCopyBlocks += transparentCopyBlocks;
    accumulatedTransformedBlocks += transformedBlocks;
    accumulatedFallbackBlocks += fallbackBlocks;
    if (measuredFrameCount >= 20) {
        const uint32_t averageComposeUs = accumulatedComposeUs / measuredFrameCount;
        const uint32_t averageTransferUs = accumulatedTransferUs / measuredFrameCount;
        const uint32_t averageFrameUs = accumulatedFrameUs / measuredFrameCount;
        Serial.printf(
            "[TileRenderer] 20 幀平均：合成 %lu.%03lu ms，TFT 傳輸 %lu.%03lu ms，總計 %lu.%03lu ms\n",
            averageComposeUs / 1000, averageComposeUs % 1000,
            averageTransferUs / 1000, averageTransferUs % 1000,
            averageFrameUs / 1000, averageFrameUs % 1000
        );
        Serial.printf(
            "[TileRenderer] cache hit=%lu miss=%lu bypass=%lu | "
            "opaque=%lu transparent=%lu transformed=%lu fallback=%lu\n",
            accumulatedCacheHits,
            accumulatedCacheMisses,
            accumulatedCacheBypasses,
            accumulatedOpaqueCopyBlocks,
            accumulatedTransparentCopyBlocks,
            accumulatedTransformedBlocks,
            accumulatedFallbackBlocks
        );
        measuredFrameCount = 0;
        accumulatedComposeUs = 0;
        accumulatedTransferUs = 0;
        accumulatedFrameUs = 0;
        accumulatedCacheHits = 0;
        accumulatedCacheMisses = 0;
        accumulatedCacheBypasses = 0;
        accumulatedOpaqueCopyBlocks = 0;
        accumulatedTransparentCopyBlocks = 0;
        accumulatedTransformedBlocks = 0;
        accumulatedFallbackBlocks = 0;
    }

#endif

    lastCameraX = camera.x;
    lastCameraY = camera.y;
    hasLastCameraPosition = true;
    return true;
}
