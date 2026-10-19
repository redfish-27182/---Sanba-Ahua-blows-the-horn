#pragma once

#include "Map/MapData.h"

class MapLoader {
public:
    // 從 SD 卡載入並檢查地圖與素材；失敗時不改變輸出資料。
    // mapPath、tilesetPath：const char*，SD 卡檔案路徑；output：MapData&，載入結果。
    // 回傳：bool，是否載入成功。
    bool load(const char *mapPath, const char *tilesetPath, MapData &output);
};
