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
    static constexpr int stripWidth = 320;
    static constexpr int stripHeight = 4;
    std::unique_ptr<uint16_t[]> strip;
};
