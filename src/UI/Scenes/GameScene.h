#pragma once
#include "Display/TileRenderer.h"
#include "EventTypes.h"

class GameScene {
public:
    // 建立遊戲畫面，借用載入完成的地圖與螢幕介面。
    // map：MapData&，地圖；display：DisplayDriver&，螢幕；無回傳值。
    GameScene(MapData &map, DisplayDriver &display) : map(map), display(display) {}
    // 接收觸控事件，計算拖曳位移。
    // event：const InputEvent&，觸控事件；無回傳值。
    void handleInput(const InputEvent &event);
    // 需要更新時繪製畫面。
    // 無輸入；回傳：bool，繪製是否成功。
    bool drawIfNeeded();
    // 要求下次重新繪製，避免對話框殘留。
    // 無輸入及回傳值。
    void requestRedraw() { dirty = true; dragging = false; }
private:
    MapData &map;
    DisplayDriver &display;
    Camera camera;
    TileRenderer renderer;
    bool dirty = true, dragging = false;
    int lastX = 0, lastY = 0;
};
