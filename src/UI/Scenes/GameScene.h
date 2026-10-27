#pragma once
#include "Display/TileRenderer.h"
#include "EventTypes.h"
#include "UI/Hud/HudLayer.h"

class GameScene {
public:
    // 建立遊戲畫面，借用載入完成的地圖與螢幕介面。
    // map：MapData&，地圖；display：DisplayDriver&，螢幕介面。
    GameScene(MapData &map, DisplayDriver &display) : map(map), display(display) {}

    // 進入遊戲時配置可重複使用的渲染緩衝區。
    // 無輸入；回傳：bool，是否成功配置。
    bool initialize();

    // 接收觸控事件，計算拖曳位移。
    // event：const InputEvent&，觸控事件。
    void handleInput(const InputEvent &event);

    // 更新 HUD 使用的天氣狀態。
    // weather：const WeatherState&，目前天氣狀態。
    void setWeather(const WeatherState &weather);

    // 檢查 HUD 中需要隨時間更新的內容。
    // 無輸入；每秒檢查一次本機 UTC 時鐘，跨分鐘時要求重繪。
    void update();

    // 需要更新時繪製畫面。
    // 回傳：bool，繪製是否成功。
    bool drawIfNeeded();

    // 要求下次重新繪製，避免對話框殘留。
    void requestRedraw() { dirty = true; dragging = false; }

private:
    MapData &map;
    DisplayDriver &display;
    Camera camera;
    TileRenderer renderer;
    HudLayer hud;
    bool dirty    = true;     // 是否需要重畫
    bool dragging = false;    // 是否正在拖曳
    int lastX = 0, lastY = 0; // 上次觸控座標
};
