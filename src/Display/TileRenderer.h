#pragma once
#include "Display/DisplayDriver.h"
#include "Game/Camera.h"
#include "Map/MapData.h"
#include <memory>

class TileRenderer {
public:
    // 進入遊戲時配置共用緩衝區，之後重繪時重複使用。
    // 無輸入；回傳：bool，是否成功配置。
    bool initialize();

    // 繪製鏡頭內的圖層。
    // map：const MapData&，地圖；camera：const Camera&，鏡頭；display：DisplayDriver&，螢幕。
    // 回傳：bool，是否成功配置緩衝區並繪製。
    bool draw(
        const MapData &map, 
        const Camera &camera, 
        DisplayDriver &display
    );

private:
    static constexpr int stripWidth = 320; // 每次繪製的圖層寬度，應與螢幕寬度相同。
    static constexpr int stripHeight = 4;  // 每次繪製的圖層高度，應為 4 的倍數以符合 DMA 傳輸要求。
    std::unique_ptr<uint16_t[]> strip; // 以 RGB565 格式存放每次繪製的圖層，大小為 stripWidth * stripHeight。
};
