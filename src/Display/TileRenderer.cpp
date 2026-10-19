#include "Display/TileRenderer.h"
#include <algorithm>
#include <memory>
#include <new>

// 按圖層合成可見像素，桃紅色為透明；不進行 SD 讀取。
// map：const MapData&，素材和地圖；camera：const Camera&，原點；display：DisplayDriver&，輸出。
// 回傳：bool，是否成功。
bool TileRenderer::draw(const MapData &map, const Camera &camera, DisplayDriver &display) {
    std::unique_ptr<uint16_t[]> strip(new (std::nothrow) uint16_t[320 * 16]);
    if (!strip) return false;
    for (int sy = 0; sy < 240; sy += 16) {
        std::fill_n(strip.get(), 320 * 16, uint16_t(0));
        for (unsigned layer = 0; layer < map.layers; ++layer) {
            for (int py = 0; py < 16; ++py) for (int px = 0; px < 320; ++px) {
                const int wx = camera.x + px, wy = camera.y + sy + py;
                if (wx >= int(map.width) * 16 || wy >= int(map.height) * 16) continue;
                const size_t pos = (size_t(layer) * map.height + wy / 16) * map.width + wx / 16;
                const uint16_t cell = map.cells[pos], code = cell & 0x1fff;
                if (!code) continue;
                auto found = std::lower_bound(map.tileIds.begin(), map.tileIds.end(), uint16_t(code - 1));
                if (found == map.tileIds.end() || *found != code - 1) continue;
                int x = wx % 16, y = wy % 16;
                // 反向查詢來源像素：先反轉水平、垂直，再反轉對角線。
                if (cell & 0x2000) x = 15 - x;
                if (cell & 0x4000) y = 15 - y;
                if (cell & 0x8000) { const int temp = x; x = y; y = temp; }
                const size_t offset = size_t(found - map.tileIds.begin()) * 512 + (y * 16 + x) * 2;
                const uint16_t color = uint16_t(map.pixels[offset] << 8) | map.pixels[offset + 1];
                if (color != 0xf81f) strip[py * 320 + px] = color;
            }
        }
        display.drawStrip(sy, 16, strip.get());
        yield();
    }
    return true;
}
