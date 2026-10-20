#include "Display/TileRenderer.h"
#include <algorithm>
#include <new>

// 配置四行 RGB565 緩衝區，保留到渲染器結束。
// 無輸入；回傳：bool，是否成功配置。
bool TileRenderer::initialize() {
    // 若已配置則直接回傳成功。
    if (strip) return true;
    
    // 記錄配置前的可用堆積記憶體大小與最大可分配區塊大小。
    const uint32_t freeBefore = ESP.getFreeHeap();
    const uint32_t largestBefore = ESP.getMaxAllocHeap();
    strip.reset(new (std::nothrow) uint16_t[stripWidth * stripHeight]);
    
    if (!strip) {
        Serial.printf(
            "[TileRenderer] allocation failed; requested=%u bytes, before: free=%u, largest=%u; after: free=%u, largest=%u\n",
            static_cast<unsigned>(stripWidth * stripHeight * sizeof(uint16_t)),
            freeBefore, 
            largestBefore,
            ESP.getFreeHeap(), 
            ESP.getMaxAllocHeap()
        );
        return false;
    }
    Serial.printf(
        "[TileRenderer] buffer allocated: %u bytes; free=%u, largest=%u\n",
        static_cast<unsigned>(stripWidth * stripHeight * sizeof(uint16_t)),
        ESP.getFreeHeap(), 
        ESP.getMaxAllocHeap()
    );
    return true;
}

// 按圖層合成可見像素，桃紅色為透明；不進行 SD 讀取。
bool TileRenderer::draw(
    const MapData &map, 
    const Camera &camera, 
    DisplayDriver &display
) {
    if (!strip) {
        Serial.println("[TileRenderer] 還沒初始化ˇ");
        return false;
    }
    for (int sy = 0; sy < 240; sy += stripHeight) {
        // 先清空緩衝區
        std::fill_n(strip.get(), stripWidth * stripHeight, uint16_t(0));
        
        // 依圖層順序合成可見像素
        for (unsigned layer = 0; layer < map.layers; ++layer) {

            // 每個區塊不跨越瓦片邊界，因此只需查找一次素材。
            for (int py = 0; py < stripHeight;) {
                const int wy = camera.y + sy + py;
                if (wy >= int(map.height) * 16) break;

                const int blockHeight = std::min(stripHeight - py, 16 - wy % 16);
                for (int px = 0; px < stripWidth;) {
                    const int wx = camera.x + px;
                    if (wx >= int(map.width) * 16) break;
                    const int blockWidth = std::min(stripWidth - px, 16 - wx % 16);
                    const size_t pos = (size_t(layer) * map.height + wy / 16) * map.width + wx / 16;
                    const uint16_t cell = map.cells[pos], code = cell & 0x1fff;
                    if (code) {
                        auto found = std::lower_bound(map.tileIds.begin(), map.tileIds.end(), uint16_t(code - 1));
                        if (found != map.tileIds.end() && *found == code - 1) {
                            const size_t tileOffset = size_t(found - map.tileIds.begin()) * 512;
                            for (int dy = 0; dy < blockHeight; ++dy) {
                                for (int dx = 0; dx < blockWidth; ++dx) {
                                    int x = wx % 16 + dx, y = wy % 16 + dy;
                                    // 反向查詢來源像素：先反轉水平、垂直，再反轉對角線。
                                    if (cell & 0x2000) x = 15 - x;
                                    if (cell & 0x4000) y = 15 - y;
                                    if (cell & 0x8000) { const int temp = x; x = y; y = temp; }
                                    const size_t offset = tileOffset + (y * 16 + x) * 2;
                                    const uint16_t color = uint16_t(map.pixels[offset] << 8) | map.pixels[offset + 1];
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
        display.drawStrip(sy, stripHeight, strip.get());
        yield();
    }
    return true;
}
