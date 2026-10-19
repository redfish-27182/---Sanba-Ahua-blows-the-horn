#pragma once
#include "Display/DisplayDriver.h"
#include "Game/Camera.h"
#include "Map/MapData.h"

class TileRenderer {
public:
    // 繪製鏡頭內的圖層。
    // map：const MapData&，地圖；camera：const Camera&，鏡頭；display：DisplayDriver&，螢幕。
    // 回傳：bool，是否成功配置緩衝區並繪製。
    bool draw(
        const MapData &map, 
        const Camera &camera, 
        DisplayDriver &display
    );
};
