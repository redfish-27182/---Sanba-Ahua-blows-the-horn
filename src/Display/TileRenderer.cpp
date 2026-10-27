#include "Display/TileRenderer.h"
#include <algorithm>
#include <new>

// 配置四行 RGB565 緩衝區，保留到渲染器結束。
// 無輸入；回傳：bool，是否成功配置。
bool TileRenderer::initialize() {
    // 若已配置則直接回傳成功。
    if (strip) return true;
    
    strip.reset(new (std::nothrow) uint16_t[stripWidth * stripHeight]);
    
    if (!strip) {
        Serial.println("[TileRenderer] strip allocation failed");
        return false;
    }

    return true;
}

// 按圖層合成可見像素，桃紅色為透明；tile 像素已常駐於 RAM。
bool TileRenderer::draw(
    const MapData &map, 
    const Camera &camera, 
    const ScreenOverlay &overlay,
    DisplayDriver &display
) {
    if (!strip) {
        Serial.println("[TileRenderer] 還沒初始化ˇ");
        return false;
    }
    for (int sy = 0; sy < SCREEN_HEIGHT; sy += stripHeight) {
        const int currentHeight = std::min(stripHeight, SCREEN_HEIGHT - sy);
        // 先清空緩衝區
        std::fill_n(strip.get(), stripWidth * stripHeight, uint16_t(0));
        
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
                        auto found = std::lower_bound(map.tileIds.begin(), map.tileIds.end(), uint16_t(code - 1));
                        if (found != map.tileIds.end() && *found == code - 1) {
                            const size_t tileIndex = size_t(found - map.tileIds.begin());
                            const size_t chunkIndex = tileIndex / TILES_PER_PIXEL_CHUNK;
                            const size_t tileInChunk = tileIndex % TILES_PER_PIXEL_CHUNK;
                            if (chunkIndex >= map.pixelChunks.size() || !map.pixelChunks[chunkIndex]) {
                                Serial.println("[TileRenderer] tile pixel chunk is unavailable");
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
                                        return false;
                                    }
                                    const uint16_t color = map.palette[paletteIndex];
                                    if (color != 0xf81f) strip[(py + dy) * stripWidth + px + dx] = color;
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
        overlay.composeStrip(strip.get(), sy, currentHeight);
        display.drawStrip(sy, currentHeight, strip.get());
        yield();
    }
    return true;
}
